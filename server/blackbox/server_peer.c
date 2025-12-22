#include <freerdp/locale/keyboard.h>
#include <freerdp/codec/color.h>
//#include <freerdp/codec/fpga.h>
//#include <freerdp/codec/host_interface.h>
#include <freerdp/hardware_manager.h>
#include <freerdp/utils/file.h>
#include <freerdp/types.h>
// #include <freerdp/bitops.h>
// #include <freerdp/utils/sleep.h>
#include <freerdp/utils/memory.h>
// #include <freerdp/utils/thread.h>
#include <freerdp/utils/profiler.h>
#include <freerdp/core_event.h>
#include "server_peer.h"
// #include "server_input.h"
// #include "sh_settings.h"
#include <corrib_logger.h>
#include <system_utils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#if 0
static void * server_peer_monitor_loop(void * arg)
{
	serverPeerContext* sp_context = (serverPeerContext* )arg;
	freerdp_peer * client = sp_context->client;

	sp_context->monitor_thread_running = true;
	sp_context->monitor_thread_state = RUNNING;

	while(sp_context->monitor_thread_running)
	{
		int ret;
		fd_set rfds_set;
		int sp_mon_queue_fd;
		struct timeval tv = { .tv_sec = DEFAULT_INTERVAL_PERIOD, .tv_usec = 0 };

		sp_mon_queue_fd = eq_get_queue_fd(sp_context->sp_mon_queue);
		if (sp_mon_queue_fd < 1)
		{
			corrib_syslog(LOG_ERR,"%s: Failed to get sp_mon_queue file descriptor for queue %p.\n",
				      __func__,sp_context->sp_mon_queue);
			break;
		}

		FD_ZERO(&rfds_set);
		FD_SET(sp_mon_queue_fd, &rfds_set);

		// Wait for next interval or an event from the main loop
		ret = select(sp_mon_queue_fd + 1, &rfds_set, NULL, NULL, &tv);
		if (ret == -1)
		{
			/* these are not really errors */
			if (((errno == EAGAIN) ||
			     (errno == EWOULDBLOCK) ||
			     (errno == EINPROGRESS) ||
			     (errno == EINTR))) /* signal occurred */
				continue;

			corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",
				      __func__,strerror(errno));
			break;
		}

		// Check if main loop has sent a request to terminate
		if (FD_ISSET(sp_mon_queue_fd, &rfds_set))
		{
			//read the queue
			eqEvent* event = eq_pop(sp_context->sp_mon_queue);
			if(event)
			{
				switch(event->type)
				{
				case EQ_EVENT_END:
				{
					// corrib_syslog(LOG_NOTICE,"%s: got an end event from server_peer, terminating.\n",
					// 	      __func__);
					eq_event_free(event); //free the event
					goto exit;
				}
				default:
					corrib_syslog(LOG_ERR,"%s: got an unknown event type:%p - %d...\n",
						      __func__,event,event->type);
					break;
				}
			}
		} //sp_mon queue

		time_t now = time(NULL);
		bool link_active = peer_check_link_activity(client);
		bbPeerContext* bb_peer_context = (bbPeerContext* )client->ContextExtra;


		if (now >= bb_peer_context->settings->expiration_time)
		{
			/*
			 * Keep-alive expired on RDP channel but if A/V channels are still
			 * receiving traffic we shouldn't abort the connection.
			 */
			if (link_active ||
			    (bb_peer_context->settings->last_active_checkpoint > (now - (2 * DEFAULT_INTERVAL_PERIOD))))
			{
				bb_peer_context->settings->expiration_time = now + DEFAULT_INTERVAL_PERIOD;
				corrib_syslog(LOG_INFO, "%s: audio/video channel activity detected within last %u seconds, postponing keepalive expiry for %u seconds",
					      __func__, (2 * DEFAULT_INTERVAL_PERIOD), DEFAULT_INTERVAL_PERIOD);
			}
			else
			{
				corrib_syslog(LOG_INFO, "%s: keepalive timeout expired and no received audio/video channel activity detected within last %u seconds, terminating connection",
					      __func__, (2 * DEFAULT_INTERVAL_PERIOD));
				corrib_syslog(LOG_ERR, "%s: NETWORK FAILURE (status expiration)\n",__func__);
				/*
				 * Disable the client socket to interrupt any socket read/write operations which may be blocking the main thread.
				 * This will also trigger termination of the main or secondary threads as they call client->CheckFileDescriptor()
				 * on each iteration, which will fail after the socket is disabled here.
				 */
				shutdown(client->sockfd, SHUT_RDWR);
				goto exit;
			}
		}

		if (bb_peer_context->settings->heartbeat_enabled)
			corrib_syslog(LOG_DEBUG,"[P%d]",client->sockfd);
	}

exit:
	sp_context->monitor_thread_running = false;
	sp_context->monitor_thread_state = STOPPED;

	// corrib_syslog(LOG_INFO,"%s: exiting thread", __func__);
	pthread_exit(NULL);
	return NULL;
}

/*
 * The server peer must monitor multiple queues, one from the listner, one from the hardware manager, and one from each peer
 */
static void * server_peer_main_loop(void * arg)
{
	serverPeerContext* sp_context = (serverPeerContext* )arg;

	freerdp_peer * client = sp_context->client;
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[32];
	fd_set rfds_set;
	sp_context->main_thread_running = true;
	int num_set;
	assert(client);
	memset(rfds, 0, sizeof(rfds));
	struct timeval tv = {.tv_sec = DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	sp_context->main_thread_state = RUNNING;

	// BOOL monitor_keepalives = (client->settings->keepalive.info_flags != 0);

	int testRDPEUSB = 0;
	int testRDPSND = 0;

	// if (monitor_keepalives)
	// {
	// 	/*
	// 	 * Spawn a separate monitor thread to check for expiry of keep-alive messages
	// 	 * received from the remote peer which would indicate loss of connectivity.
	// 	 *
	// 	 * A separate thread is needed to avoid having these checks delayed by blocking
	// 	 * I/O operations such as a socket send() which can be blocked for up to 15
	// 	 * minutes when a network link goes down.
	// 	 */
	// 	sp_context->monitor_thread = server_peer_create_thread(server_peer_monitor_loop,
	// 							       sp_context, "monitor");
	// 	if (sp_context->monitor_thread == -1)
	// 	{
	// 		corrib_syslog(LOG_ERR, "%s: failed to create monitor thread: %s\n",
	// 			      __func__, strerror(errno));
	// 		goto exit;
	// 	}
	// 	server_peer_set_specific_thread_priority(sp_context->monitor_thread, SCHED_RR, 0);
	// }

	//corrib_syslog(LOG_INFO,"Starting server main loop for peer %s in %s\n",sp_context->client->hostname,__func__);

	while(sp_context->main_thread_running)
	{

		rcount = 0;
		server_peer_get_timeout_interval(client, &tv);
		if (server_peer_get_main_fds(sp_context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get server peer file descriptors in %s\n",__func__);
			sp_context->main_thread_running = false;
			sp_context->main_thread_state = STOPPED;
			break;
		}
		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
		{
			sp_context->main_thread_running = false;
			sp_context->main_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			break;
		}
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",  __func__,strerror(errno));
				sp_context->main_thread_running = false;
				sp_context->main_thread_state = STOPPED;
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				break;
			}
		} //everything is as we expected
		else
		{
			int cm_peer_fd = eq_get_queue_fd(sp_context->cm_peer_queue);
#ifndef TWO_THREADS
			if (client->CheckFileDescriptor(client) != true)  //this is the checking of the actual RDP transport 
			{
				corrib_syslog(LOG_INFO,"%s: Failed to check freerdp file descriptor for Peer, terminating peer. The peer may have left the connection\n", __func__);
				/* this can indicate a loss of connection to the client side */
				sp_context->main_thread_running = false;
				sp_context->main_thread_state = STOPPED;
				//okay we are terminating the connection for known or unknown reasons
				//we need to encure there are no outstanding signals that need to be processed before we go
				//search the queue for the relevant events and act on them if required
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				break;
			}
#endif
			if (FD_ISSET(cm_peer_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
				eqEvent* event = eq_pop(sp_context->cm_peer_queue);
				if(event)
				{
					if(sp_context->client->settings->performance_analysis)
					{
						event->receive_time = sh_log_get_mstime();
					}
					switch(event->type)
					{
					case EQ_EVENT_END: //this is only issued by the secondary_peer
						//corrib_syslog(LOG_INFO,"%s: got an end event, terminating.\n",  __func__,strerror(errno));
						sp_context->main_thread_running = false;
						sp_context->main_thread_state = STOPPED;
						//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
						//server_peer_deinit(sp_context);
						eq_push(sp_context->sp_sp_queue, (eqEvent *)event);
						break;
					case EQ_EVENT_ERROR_INFO:
					{
						EventErrorInfo * event_error_info = (EventErrorInfo *)event;
						corrib_syslog(LOG_ERR,"Sending client error info: %d\n",event_error_info->error_code);

						// client->SendErrorInfoData(client,event_error_info->error_code);

						break;
					}
// 					case EQ_EVENT_AUDIO_COMMAND_AVAILABLE:
// 					{
// #if 0
// 						//corrib_syslog(LOG_DEBUG,"server_peer_main_loop: GOT AUDIO COMMAND\n");
// #endif
// 						EventAudioCommandAvailable *audio_command_available_event = (EventAudioCommandAvailable *)event;
// 						assert(sp_context);
// 						pthread_mutex_lock(&sp_context->rdpsnd_mutex);
// 						if(sp_context->rdpsnd != NULL)
// 							sp_context->rdpsnd->ProcessCommand(sp_context->rdpsnd, audio_command_available_event->cmd);
// 						pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
// 						audio_data_command_free(audio_command_available_event->cmd);
// 						break;
// 					}
// 					case EQ_EVENT_USB_COMMAND_AVAILABLE:
// 					{
// 						bb_usbr_debug("sp_context: %p /*sp_context->*/rdpeusb: %p", sp_context, /*sp_context->*/rdpeusb);
// 						EventUsbCommandAvailable *usb_command_available_event = (EventUsbCommandAvailable *)event;


// 						if(/*sp_context->*/rdpeusb != NULL)
// 						{
// 							if(/*sp_context->*/rdpeusb->ProcessHostCommand(/*sp_context->*/rdpeusb,
// 																		usb_command_available_event->sequence_data,
// 																		usb_command_available_event->device_id,
// 																		usb_command_available_event->cmd))
// 							{
// 								/* UnRecoverable Error Processing USB Command - exit connection ... */
// 								corrib_syslog(LOG_ERR,"Error Processing USB Command ...............in. %s\n",__func__);
// 								//sp_context->main_thread_running = false;
// 								//sp_context->main_thread_state = STOPPED;
// 								//break;
// 							}
// 						}
// 						else
// 						{
// 							bb_usbr_debug("/*sp_context->*/rdpeusb is NULL: Releasing USB event");
// 							usb_command_available_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
// 							eq_push(sp_context->peer_cm_queue, (eqEvent *)usb_command_available_event);
// 							event = NULL;
// 							//usb_command_free(usb_command_available_event->cmd);
// 						}
// 						break;
// 					}
					// case EQ_EVENT_DESKTOP_RESIZE:
					// {
					// 	EventDesktopResize* event_desktop_resize = (EventDesktopResize*)event;
					// 	rdpUpdate* update = client->update;
					// 	update->DesktopResize(update->context,event_desktop_resize->width,
					// 			event_desktop_resize->height,event_desktop_resize->refresh,event_desktop_resize->sync_loss,
					// 			sp_context->client->video_slave_cid,sp_context->client->video_sequence_number);
					// 	break;
					// }
					default:
						corrib_syslog(LOG_ERR,"%s: got an unknown event type:%d.\n",  __func__,event->type);
						break;
					}
					eq_event_free(event); //free the event
				}


			} //cm_peer event
			int sp_sp_fd = eq_get_queue_fd(sp_context->sp_sp_queue);
			if (FD_ISSET(sp_sp_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
				eqEvent* event = eq_pop(sp_context->sp_sp_queue);
				if(sp_context->client->settings->performance_analysis)
				{
					event->receive_time = sh_log_get_mstime();
				}
				if(event)
				{
					switch(event->type)
					{
						case EQ_EVENT_END:
						{
							corrib_syslog(LOG_NOTICE,"%s: got an end event from sp_sp queue, terminating.\n",  __func__);
							sp_context->client->terminating = true;
							sp_context->main_thread_running = false;
							sp_context->main_thread_state = STOPPED;
							//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
							//server_peer_deinit(sp_context);
							eq_event_free(event); //free the event
							break;
						}
						default:
							corrib_syslog(LOG_ERR,"%s: got an unknown event type:%d..\n",  __func__,event->type);
							break;
					}
				}
			} //sp_sp queue

			// if (sp_context->main_thread_running) //ARPM: Checking USBR file descriptors
			// {
			// 	if (WTSVirtualChannelManagerCheckFileDescriptor(sp_context->vcm))
			// 	{
			// 		pthread_mutex_lock(&sp_context->rdpsnd_mutex);
			// 		if (sp_context->rdpsnd)
			// 		{
			// 			if (!sp_context->rdpsnd->CheckFileDescriptor(sp_context->rdpsnd))
			// 			{
			// 				testRDPSND = -1;
			// 				corrib_syslog(LOG_ERR,"%s DEBUG_ENABLEDs: Failed to read rdpsnd file descriptor.\n", __func__);
			// 				break;
			// 			}
			// 		}
			// 		pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
			// 		if (/*sp_context->*/rdpeusb)
			// 		{
			// 			if (!/*sp_context->*/rdpeusb->CheckFileDescriptors(/*sp_context->*/rdpeusb))
			// 			{
			// 				testRDPEUSB = -1;
			// 				corrib_syslog(LOG_ERR,"%s: Failed to read rdpeusb file descriptor.\n", __func__);
			// 				break;
			// 			}
			// 		}
			// 	}
			// }
		}

		if ((tv.tv_sec == 0) && (tv.tv_usec == 0))
		{
			client->settings->interval_time = time(NULL) + DEFAULT_INTERVAL_PERIOD;

			/* Send a new keepalive message periodically to the remote peer to
			 * let them know we're still connected. */
			if (client->activated)
				peer_send_cloudium_pdu(client);
		}
	} //end of while loop

exit:
#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG, "%s: Main Loop terminating: DEV FLAGS - testRDPSND: %d testRDPEUSB: %d\n", __func__, testRDPSND, testRDPEUSB);

	corrib_syslog(LOG_DEBUG,"%s:main_loop terminating: sp_context->main_thread_running =%d",__func__,sp_context->main_thread_running );
#endif
	//we now need to free any resources associated with this client
	//The connection manager created the queues for this peer so it is responsible for purging and destroying the queues
	//The CM needs to know about this event because it may need to inform multicast layers and or multiple unicast connections and in any event remove the peer from the list of active peers

	//if(sp_context->secondary_thread_running)
	//{
	//	//corrib_syslog(LOG_INFO,"%s:requesting secondary thread to complete.",__func__);
	//	EventEnd* event_end = event_end_new();
	//	eq_push(sp_context->sp_sp_queue, (eqEvent *)event_end);
	//	//corrib_syslog(LOG_INFO,"%s:complete pushed.",__func__);
	//	while(sp_context->secondary_thread_running) { printf("."); sleep(1); }
	//}

	// if (monitor_keepalives)
	// {
    //     #ifdef DEBUG_ENABLED
	// 	corrib_syslog(LOG_INFO, "%s: requesting monitor thread to complete.", __func__);
    //     #endif
	// 	EventEnd* event_end = event_end_new();
	// 	eq_push(sp_context->sp_mon_queue, (eqEvent *)event_end);
	// 	pthread_join(sp_context->monitor_thread, NULL);
	// }

	bb_usbr_debug("#### server_peer_deinit ####");
	server_peer_deinit(sp_context);


	//TODO RMC, need to add the peer_cide to this event
	EventPeerTerminated* peer_terminated_event =  event_peer_terminated_new(client->sockfd,0); //create an end event to stop the hardware manager
	peer_terminated_event->frame_processing_h1 = sp_context->frame_processing_h1;
	peer_terminated_event->frame_processing_h2 = sp_context->frame_processing_h2;
	eq_push(sp_context->peer_cm_queue, (eqEvent *)peer_terminated_event);

	pthread_exit(NULL);
	return NULL;

}

//Main Processing Loop and thread management
//-------------------------------------------
static void server_peer_run(serverPeerContext* server_peer_context)
{
    int main_thread_wait_count = 0;

    if(server_peer_context)
    {

        server_peer_context->main_thread = server_peer_create_thread(server_peer_main_loop,server_peer_context,"main");
#ifdef TWO_THREADS
        //
        // The code logic requires the 'main_loop' to be running before the 'secondary_loop' (BUG-6088)
        // This addition will ensure that this always happens.
        // 
        while((!server_peer_context->main_thread_running) && (main_thread_wait_count < MAX_ML_WAIT_COUNT))
        {
            usleep(200);
            main_thread_wait_count += 1;
        }
        corrib_syslog(LOG_DEBUG,"%s():- Waited (200 * %d (ns))for server_peer_main_thread to start before starting server_peer_secondary_thread.",__func__,main_thread_wait_count);

        if(main_thread_wait_count == MAX_ML_WAIT_COUNT)
        {
            corrib_syslog(LOG_ERR,"%s():- server_peer_main_thread hasn't started within the wait time - continuing in any case...",__func__);
        }

        server_peer_context->secondary_thread = server_peer_create_thread(server_peer_secondary_loop,server_peer_context,"secondary");
#endif
        if(server_peer_context->main_thread == -1 || (server_peer_context->secondary_thread == -1))
        {
            corrib_syslog(LOG_ERR,"Server Peer could not create threads in %s, terminating\n",__func__);
            exit(0);
        }
        else
        {
            pthread_detach(server_peer_context->main_thread);
#ifdef TWO_THREADS
            pthread_detach(server_peer_context->secondary_thread);
#endif
        }
#ifdef TWO_THREADS
        server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_RR,0); //set the video thread as low as possible
        server_peer_set_specific_thread_priority(server_peer_context->secondary_thread,SCHED_FIFO,99); //set the mouse handling as high as possible
#else
        server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_FIFO,99);
#endif
        //server_peer_set_specific_thread_priority(server_peer_context->secondary_thread,SCHED_RR,0); //set the video thread as high as possible
        //server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_FIFO,99); //set the mouse handling as low as possible
    }
    else
        corrib_syslog(LOG_ERR,"Server Context not initialised in %s\n",__func__);

}
#endif

BOOL server_peer_context_new(freerdp_peer* client, serverPeerContext* peer_context)
{
	bbPeerContext* bb_peer_context;

	if (!client) {
		corrib_syslog(LOG_DEBUG, "%s(): NULL pointer at %d\n", __func__, __LINE__);
		return;
	}

	corrib_syslog(LOG_DEBUG,"SP: %s(): freerdp_peer* client at line %d = %p\n", __func__, __LINE__, client);
	
	bb_peer_context = (bbPeerContext* )client->ContextExtra;
	if (!bb_peer_context) {
		corrib_syslog(LOG_DEBUG, "%s(): NULL pointer at %d\n", __func__, __LINE__);
		return;
	}

	corrib_syslog(LOG_DEBUG,"SP: %s(): bbPeerContext * bb_peer_context at line %d = %p\n", __func__, __LINE__, bb_peer_context);

	corrib_syslog(LOG_DEBUG, "%s(): Peer init at %d\n", __func__, __LINE__);
	// bb_peer_context->update->context = client->context;
	// bb_peer_context->input->context = client->context;
	bb_peer_context->terminating = false;
	bb_peer_context->video_master_cid = 0x02;
	bb_peer_context->video_slave_cid = 0;
	bb_peer_context->video_sequence_number = 0;
	bb_peer_context->audio_master_cid = 0x03;
	bb_peer_context->audio_slave_cid = 0;
	bb_peer_context->audio_sequence_number=0;
	bb_peer_context->client_ready_count = 0;
	bb_peer_context->connection_mode = UNKNOWN_CONNECTION_MODE;

	corrib_syslog(LOG_DEBUG, "%s(): at %d\n", __func__, __LINE__);
	//peer_context->info = sh_info_init();
	//peer_context->rfx_context = rfx_context_new();

	peer_context->activated = false;
	peer_context->cm_peer_queue = bb_peer_context->cm_peer_queue; //we take a reference to the queue created by the client for this peer
	peer_context->peer_cm_queue = bb_peer_context->peer_cm_queue; //we take a reference to the queue created by the connection manager for all peers


	// peer_context->vcm = WTSCreateVirtualChannelManager(client);

	if (freerdp_check_file_exists("/usr/local/NTO"))
	{
		corrib_syslog(LOG_DEBUG, "%s(): #### Disabling keep alive ####\n", __func__);
		bb_peer_context->settings->expiration_time = LONG_MAX;
		bb_peer_context->settings->keepalive.info_flags = 0;
	}
	corrib_syslog(LOG_DEBUG, "%s(): at %d\n", __func__, __LINE__);
	// pthread_mutex_init(&peer_context->rdpsnd_mutex, NULL);
	return TRUE;
}

void server_peer_context_free(freerdp_peer* client, serverPeerContext* context)
{
#if 0
	if (context)
	{
		/*
		shpeer_profiler_print(client);
		sh_encode_profiler_print(client);
		sh_encode_profiler_free(client);
		sh_peer_profiler_free(client);
		*/

		pthread_mutex_lock(&context->rdpsnd_mutex);
		rdpsnd_server_context_free(context->rdpsnd);
		context->rdpsnd = NULL;
		pthread_mutex_unlock(&context->rdpsnd_mutex);
		pthread_mutex_destroy(&context->rdpsnd_mutex);

		WTSDestroyVirtualChannelManager(context->vcm);
		if(context->sp_sp_queue != NULL) //FIXME,  this needs to be in a destructor
		{
			//corrib_syslog(LOG_INFO,"%s: destroying sp_sp queue\n",__func__);
#ifdef MEMORY_ALLOCATION_MONITOR
                        usleep(200); 
                        corrib_syslog(LOG_DEBUG,"%s: freeing sp_sp_queue memory\n",__func__);
			eq_queue_free(context->sp_sp_queue,__func__);
#else
			usleep(200); //this is an ugly hack for bug BUG 1682. This sets this thread on the right side of a race, the correct solution requires us to remove the secondary thread

			eq_queue_free(context->sp_sp_queue);
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_sp_queue\n",__func__);
            #endif
#endif
			context->sp_sp_queue = NULL;
		}
		if(context->sp_mon_queue != NULL) //FIXME,  this needs to be in a destructor
		{
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_INFO,"%s: destroying sp_mon queue\n",__func__);
            #endif
#ifdef MEMORY_ALLOCATION_MONITOR
                        corrib_syslog(LOG_DEBUG,"%s: freeing sp_mon_queue memory\n",__func__);
			eq_queue_free(context->sp_mon_queue,__func__);
                        corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_mon_queue\n",__func__);
#else
			eq_queue_free(context->sp_mon_queue);
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_mon_queue\n",__func__);
            #endif
#endif
			context->sp_mon_queue = NULL;
		}
	}
#endif
}

void server_peer_init(freerdp_peer* client, cmContext* cm_context)
{
	bbPeerContext* bb_peer_context;
	bb_peer_context = (bbPeerContext* )client->ContextExtra;
	if (!bb_peer_context) {
		corrib_syslog(LOG_DEBUG, "%s(): NULL pointer at %d\n", __func__, __LINE__);
		return;
	}

	client->ContextSize = sizeof(serverPeerContext); //this will be used by freerdp_peer_context_new to allocate necessary space for the client context
	client->ContextNew = (psPeerContextNew) server_peer_context_new;
	client->ContextFree = (psPeerContextFree) server_peer_context_free; //this will be invoked from freerdp_peer_context_free

	freerdp_peer_context_new(client); //this will force a call to server_peer_context_new through the ContextNew callback, after this client->context is valid
	corrib_syslog(LOG_DEBUG, "%s(): at %d\n", __func__, __LINE__);
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	sp_context->client = client;
	//---------------------------------------
	rdpSettings* settings;

	if (!client->context->settings) {
		corrib_syslog(LOG_ERR, "%s(): No settings at %d\n", __func__, __LINE__);
		return;
	}
	else {
		settings = client->context->settings; //bb_peer_context->settings;
	}

	corrib_syslog(LOG_DEBUG, "%s(): at %d\n", __func__, __LINE__);

	// if(cm_context->hm_context->head_detected[0] && cm_context->hm_context->head_detected[1])
	// 	settings->num_monitors_detected = 2;
	// else

	// bb_peer_context->settings 

	// freerdp_settings_set_uint32(settings, FreeRDP_num_monitors_detected, 0xCACA);

	settings->num_monitors_detected = 1;

	// UINT32 toto = freerdp_settings_get_uint32(settings, FreeRDP_num_monitors_detected);


	// settings->num_monitors_detected = toto;

	//All of these here are subject to a race with server_peer_post_connect
	//TODO MD - We need to set these are we know what the client is
	//settings->connection_resolution[FIRST_HEAD] = *cm_context->hm_context->connection_resolution[FIRST_HEAD];
	//settings->connection_resolution[SECOND_HEAD] = *cm_context->hm_context->connection_resolution[SECOND_HEAD];
	// settings->head_detected[FIRST_HEAD] = cm_context->hm_context->head_detected[FIRST_HEAD];
	// settings->head_detected[SECOND_HEAD] = cm_context->hm_context->head_detected[SECOND_HEAD];

	// settings->performance_analysis = cm_context->performance_analysis;
	// settings->debug_enabled = cm_context->debug_enabled;
	settings->cert_file = xstrdup("/opt/blackbox/shfreerdp/server.crt");
	settings->privatekey_file = xstrdup("/opt/blackbox/shfreerdp/server.key");
	settings->rdp_key_file = xstrdup("/opt/blackbox/shfreerdp/rdp.key");
	corrib_syslog(LOG_DEBUG, "%s(): at %d\n", __func__, __LINE__);
#if 0
	settings->nla_security = false;
	settings->rfx_codec = true;
	//Callbacks
	client->PostConnect = server_peer_post_connect;
	client->SignalClientReady = server_peer_signal_client_ready;
	client->SignalMulticastInfo = server_peer_signal_multicast_info;
	client->SignalResChangeComplete = server_peer_signal_res_change_complete;
	client->SignalAccessStatus = server_peer_signal_access_status;
	client->SignalRecoveryRequest = server_peer_signal_recovery_request;
	//client->SendCloudiumMessage = server_peer_send_cloudium_message; //should only be called from the peer

	/*
	 *	USBR Initialization
	 */
	bb_usbr_debug("Callback init: InitialiseUsbrServer CloseUsbrChannel");
	client->InitialiseUsbrServer = server_peer_initialise_rdpeusb_server;
	client->CloseUsbrChannel = server_peer_close_rdpeusb_channel;
	/*****/

	client->Capabilities = server_peer_capabilities;
	client->Activate = server_peer_activate;
	client->Signal = server_peer_signal;
	client->ReadOutputReport = server_peer_read_output_report;
	client->video_channel = -1;
	client->audio_channel = -1;
	client->video_slave_cid = 0;
	client->audio_slave_cid = 0;
	client->last_rtt = 0;
	client->last_mss = 0;

	server_input_register_callbacks(client->input);
	//client->peer_type = PRIMARY_PEER;
	client->connection_state = PEER_CONNECTION_STATE_INIT;
	client->Initialize(client);
	//---------------------------------------
#ifdef MEMORY_ALLOCATION_MONITOR
    char id[255];
    snprintf(id,255,"%s_sp_sp_queue",__func__);
    sp_context->sp_sp_queue = eq_queue_new(id);
    snprintf(id,255,"%s_sp_mon_queue",__func__);
    sp_context->sp_mon_queue = eq_queue_new(id);
#else
    sp_context->sp_sp_queue = eq_queue_new();
    sp_context->sp_mon_queue = eq_queue_new();
#endif
    eq_set_name(sp_context->sp_sp_queue,"sp_sp_queue");
    eq_set_name(sp_context->sp_mon_queue,"sp_mon_queue");
	server_peer_run(sp_context);
#endif
}

void server_peer_accepted(cmContext* cm_context, freerdp_peer* client)
{
	corrib_syslog(LOG_DEBUG, "%s(): Server Peer initialising\n", __func__);

	server_peer_init(client, cm_context);
}

#if 0
#include <freerdp/locale/keyboard.h>
#include <freerdp/codec/color.h>
//#include <freerdp/codec/fpga.h>
//#include <freerdp/codec/host_interface.h>
#include <freerdp/hardware_manager.h>
#include <freerdp/utils/file.h>
#include <freerdp/types.h>
#include <freerdp/bitops.h>
#include <freerdp/utils/sleep.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/thread.h>
#include <freerdp/utils/profiler.h>
#include <freerdp/core_event.h>
#include "server_peer.h"
#include "server_input.h"
#include "sh_settings.h"
#include <corrib_logger.h>
#include <system_utils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>

//#define USBR_DEBUG
#if defined(USBR_DEBUG) || defined(DEBUG_ENABLED)
#define bb_usbr_debug(fmt,args...) \
	corrib_syslog(LOG_DEBUG, "%s(): [USBR] " fmt,  __func__, ## args)
#else
#define bb_usbr_debug(fmt,args...) \
	do { } while (0)
#endif /* USBR_DEBUG */

#define MAX_FDS 32
#define MAX_ML_WAIT_COUNT 100 // Max iterations of 100ns loop to wait for the server peer main thread to start
#define TWO_THREADS
//#define SHARED_MODE_DEBUG 1
//#define DEBUG_ENABLED 1

static void * server_peer_main_loop(void * arg);
static void * server_peer_secondary_loop(void * arg);
static void * server_peer_monitor_loop(void * arg);
static void server_peer_deinit(serverPeerContext* sp_context);
static pthread_t server_peer_create_thread( void* func, void* arg,char thread_type[]);
static void server_peer_run(serverPeerContext* server_peer_context);
static boolean server_peer_post_connect(freerdp_peer* client);
//DOMAIN_KEY
static boolean server_peer_signal_client_ready(freerdp_peer* client, uint32 status,boolean preemption,uint8 * domain_key,uint32 session_id,char * loggedin_user);
static boolean server_peer_send_cloudium_message(freerdp_peer* client1, uint32 command,freerdp_peer* client2,char * username);
static void server_peer_set_specific_thread_priority(pthread_t thread_id, int new_policy,int new_priority);

/*
 * [USBR]
 * Static reference for USBr plugin context. Plugin will always use internal vcm pointer for rdp communication.
 * 
 */
rdpeusbServerContext *rdpeusb = NULL;
static int rdpeusb_already_init_flag = 0;


/**
 * Server setting are not initialised at this point
 */
void server_peer_context_new(freerdp_peer* client, serverPeerContext* peer_context)
{
	//peer_context->info = sh_info_init();
	//peer_context->rfx_context = rfx_context_new();

	peer_context->activated = false;
	peer_context->cm_peer_queue = client->cm_peer_queue; //we take a reference to the queue created by the client for this peer
	peer_context->peer_cm_queue = client->peer_cm_queue; //we take a reference to the queue created by the connection manager for all peers


	peer_context->vcm = WTSCreateVirtualChannelManager(client);

	if (freerdp_check_file_exists("/usr/local/NTO"))
	{
		corrib_syslog(LOG_DEBUG, "%s(): #### Disabling keep alive ####\n", __func__);
		client->settings->expiration_time = LONG_MAX;
		client->settings->keepalive.info_flags = 0;
	}

	pthread_mutex_init(&peer_context->rdpsnd_mutex, NULL);
}


/*
 * This is called via a callback from freerdp_peer_context_free
 */
void server_peer_context_free(freerdp_peer* client, serverPeerContext* context)
{
	if (context)
	{
		/*
		shpeer_profiler_print(client);
		sh_encode_profiler_print(client);
		sh_encode_profiler_free(client);
		sh_peer_profiler_free(client);
		*/

		pthread_mutex_lock(&context->rdpsnd_mutex);
		rdpsnd_server_context_free(context->rdpsnd);
		context->rdpsnd = NULL;
		pthread_mutex_unlock(&context->rdpsnd_mutex);
		pthread_mutex_destroy(&context->rdpsnd_mutex);

		WTSDestroyVirtualChannelManager(context->vcm);
		if(context->sp_sp_queue != NULL) //FIXME,  this needs to be in a destructor
		{
			//corrib_syslog(LOG_INFO,"%s: destroying sp_sp queue\n",__func__);
#ifdef MEMORY_ALLOCATION_MONITOR
                        usleep(200); 
                        corrib_syslog(LOG_DEBUG,"%s: freeing sp_sp_queue memory\n",__func__);
			eq_queue_free(context->sp_sp_queue,__func__);
#else
			usleep(200); //this is an ugly hack for bug BUG 1682. This sets this thread on the right side of a race, the correct solution requires us to remove the secondary thread

			eq_queue_free(context->sp_sp_queue);
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_sp_queue\n",__func__);
            #endif
#endif
			context->sp_sp_queue = NULL;
		}
		if(context->sp_mon_queue != NULL) //FIXME,  this needs to be in a destructor
		{
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_INFO,"%s: destroying sp_mon queue\n",__func__);
            #endif
#ifdef MEMORY_ALLOCATION_MONITOR
                        corrib_syslog(LOG_DEBUG,"%s: freeing sp_mon_queue memory\n",__func__);
			eq_queue_free(context->sp_mon_queue,__func__);
                        corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_mon_queue\n",__func__);
#else
			eq_queue_free(context->sp_mon_queue);
            #ifdef DEBUG_ENABLED
			corrib_syslog(LOG_DEBUG,"%s:Freed context->sp_mon_queue\n",__func__);
            #endif
#endif
			context->sp_mon_queue = NULL;
		}






	}
}

//Callbacks
//------------------------------------------------

boolean server_peer_capabilities(freerdp_peer* client)
{

	return true;
}


static boolean server_peer_send_cloudium_message(freerdp_peer* client1, uint32 command,freerdp_peer* client2,char * username)
{
	if(client2)
		rdp_send_cloudium_general_info_pdu(client1->context->rdp,command,client2->hostname,username);
	else
		rdp_send_cloudium_general_info_pdu(client1->context->rdp,command,"10.10.10.10",username);
	return true;
}

static boolean server_peer_signal_access_status(freerdp_peer* client,ACCESS_STATUS status,char * username)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventAccessStatus * client_access_status = event_peer_access_status_new(client->sockfd,status);
	strncpy(client_access_status->username,username,MAX_USERNAME_LENGTH);
	client_access_status->access_status = status;
	//corrib_syslog(LOG_DEBUG,"%s: access_status was %u\n",__func__,status);
	if(client->settings->performance_analysis)
		client_access_status->send_time = sh_log_get_mstime();

	eq_push(sp_context->peer_cm_queue,(eqEvent *)client_access_status); //tell the connection manager about this
	//corrib_syslog(LOG_DEBUG,"%s:client is ready event on queue\n",__func__);
	return true;
}

static boolean server_peer_signal_recovery_request(freerdp_peer* client,int head)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventRecoveryRequest * recovery_request = event_peer_recovery_request_new(client->sockfd,head);

	if(client->settings->performance_analysis)
		recovery_request->send_time = sh_log_get_mstime();

	eq_push(sp_context->peer_cm_queue,(eqEvent *)recovery_request); //tell the connection manager about this
	//corrib_syslog(LOG_DEBUG,"%s:client is ready event on queue\n",__func__);
	return true;
}

static boolean server_peer_signal_multicast_info(freerdp_peer* client, uint32 multicast_status,boolean preemption,uint8 * domain_key,uint32 session_id)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventConnectionInfo * connection_info = event_connection_info_new(client->sockfd,multicast_status); //client id
	eq_push(sp_context->peer_cm_queue,(eqEvent *)connection_info); //tell the connection manager about this
    #if DEBUG_ENABLED
	corrib_syslog (LOG_INFO,"%s:connection_info event on queue\n",__func__);
    #endif
	return true;
}



//DOMAIN_KEY
static boolean server_peer_signal_client_ready(freerdp_peer* client, uint32 connection_type,boolean preemption,uint8 * domain_key,uint32 session_id,char * loggedin_user)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventClientReady * client_ready_event = event_client_ready_new(client->sockfd,connection_type,preemption,domain_key,session_id,loggedin_user); //client id

	eq_push(sp_context->peer_cm_queue,(eqEvent *)client_ready_event); //tell the connection manager about this
    #if DEBUG_ENABLED
	corrib_syslog (LOG_INFO,"%s:client is ready event on queue\n",__func__);
    #endif
	return true;
}

static boolean server_peer_signal_res_change_complete(freerdp_peer* client, uint32 multicast_status, int head)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventResChangeComplete * client_res_change_complete = event_res_change_complete_new(client->sockfd,multicast_status); //client id
	bool sync_loss = false;

	client_res_change_complete->client = client;
	client_res_change_complete->head = head;
	corrib_syslog(LOG_DEBUG,"%s: multicast_status %u\n",__func__,multicast_status);
	if(client->settings->performance_analysis)
		client_res_change_complete->send_time = sh_log_get_mstime();

	sync_loss = (client->settings->connection_resolution[head-1].width == 0) && (client->settings->connection_resolution[head-1].height == 0);

	//we do not want to do this on a sync loss
	if (sync_loss) //do not restart on sync loss only when we have video
	{
		corrib_syslog(LOG_DEBUG,"%s: sync loss on head %d\n", __func__, head);
		client_res_change_complete->sync_loss =1;
	}
	else
	{
		client_res_change_complete->sync_loss = 0;
		if(multicast_status)
		{
			//TODO MD Why are we setting this here?
			client->connection_mode = MULTICAST_MODE;
		}
		else
		{
			//This is a normal connection
			client->connection_mode = UNICAST_MODE;
			corrib_syslog (LOG_INFO,"Resolution change in %s %dx%d %dHz\n",__func__,client->settings->connection_resolution[head-1].width,
				client->settings->connection_resolution[head-1].height,client->settings->connection_resolution[head-1].refresh);

		}
	}
	eq_push(sp_context->peer_cm_queue,(eqEvent *)client_res_change_complete); //tell the connection manager about this
    #ifdef DEBUG_ENABLED
	sh_syslog (LOG_INFO,"%s:client_ID:%s res change complete on queue\n",__func__,client->hostname);
    #endif
	return true;
}
/* RDP Sound activation callback */
static void server_peer_rdpsnd_activated(rdpsnd_server_context* context)
{
	corrib_syslog (LOG_INFO,"RDP Sound activated in this connection.\n");

}


/* RDPEUSB open rdpeusb channels callback */
static boolean server_peer_initialise_rdpeusb_server(freerdp_peer* client)
{
	boolean ret;

	bb_usbr_debug("client*: %p client->context*: %p client_ID:%s Initialising RDPEUSB Server",	client,
																									client->context, 
																									client->hostname);
	if (!rdpeusb)
		return false;

	serverPeerContext * sp_context = (serverPeerContext *)client->context;

	if (sp_context->vcm != rdpeusb->vcm) {
		bb_usbr_debug("sp %p rdpeusb %p", sp_context->vcm,  rdpeusb->vcm);
		return false;
	}

	bb_usbr_debug("rdpeusb->Initialize(rdpeusb)");

	ret = /*sp_context->*/rdpeusb->Initialize(/*sp_context->*/rdpeusb);

	bb_usbr_debug("end");
	return ret;
}

/* RDPEUSB close rdpeusb channels callback */
static boolean server_peer_close_rdpeusb_channel(freerdp_peer* client, int channelId)
{
	bb_usbr_debug("client: %p client->hostname: %s - Closing RDPEUSB channel %d", client, 
																			client->hostname,
																			channelId);
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	return /*sp_context->*/rdpeusb->ChannelClose(/*sp_context->*/rdpeusb, channelId);
}

/* RDPEUSB process command callback */

/*
 * ARPM: All USBR events sent RX are processed here
 */
static void server_peer_process_rdpeusb_device_command(rdpeusbServerContext* context, sequenceData* sequence_data, int device_id, void* cmd)
{
#ifdef DEBUG_ENABLED
	printf("In server_peer_process_rdpeusb_device_command Callback type %d ptr = %p \n", sequence_data->type, sequence_data);
#endif
	switch(sequence_data->type) {
		case USBSQT_RELEASE_COMMAND:
		case USBSQT_REQUEST_RESOURCE:
		case USBSQT_CONFIGURE_RESOURCE:
		case USBSQT_IO_CONTROL_COMPLETION:
		case USBSQT_URB_COMPLETION_NO_DATA:
		case USBSQT_URB_COMPLETION_DATA:
		case USBSQT_RELEASE_RESOURCE:
		{
			serverPeerContext * sp_context = (serverPeerContext *)context->vcm->client->context;
			EventUsbCommandAvailable * usb_event = event_usb_command_available_new(sequence_data, device_id, cmd);
			if (usb_event == NULL) {
				corrib_syslog(LOG_ERR, "%s(): event_usb_command_available_new().error: type = %d\n",  __func__, sequence_data->type);
			}
			else {
				if (sp_context->client->settings->performance_analysis) {
					usb_event->send_time = sh_log_get_mstime();
				}
				eq_push(sp_context->peer_cm_queue, (eqEvent *)usb_event);
			}
		}
		break;
		default:

			corrib_syslog (LOG_ERR, "%s(): Got an unknown sequence type: [sequence_data->type = %d]\n",  __func__, sequence_data->type);
		break;
	}

}

/*
	We want the context to be created for the first peer (with USBr enabled) only.
 */
void server_peer_create_rdpeusb_context_and_set_cbs(serverPeerContext* sp_context, rdpeusbServerContext *__rdpeusb)
{
	bb_usbr_debug("begin");
	if (rdpeusb) {
		bb_usbr_debug("rdpeusb is not null, preventing another creation of rdpeusb server context");
		return;
	}

	// ARPM: This ation will save the vcm to an internal variable in rdpeusb context.
	/*sp_context->*/rdpeusb = rdpeusb_server_context_new(sp_context->vcm);

	bb_usbr_debug("sp %p rdpeusb %p", sp_context->vcm,  rdpeusb->vcm);

	/*sp_context->*/rdpeusb->ProcessDeviceCommand = server_peer_process_rdpeusb_device_command;

	bb_usbr_debug("end");
	return;
}

/*
	Context gets release when first connected peer (with USBr enabled) leaves only
 */
void server_peer_release_rdpeusb_context(serverPeerContext* sp_context, rdpeusbServerContext *__rdpeusb)
{
	bb_usbr_debug("begin");
	if (!rdpeusb)
		return;

	// ARPM: Releasing rdpeusb context only if leaving server peer vcm is the same as the one saved in rdpeusb
	if (sp_context->vcm == rdpeusb->vcm) {
		bb_usbr_debug("(sp_context->vcm == rdpeusb->vcm)");
		// if (rdpeusb_already_init_flag) {
			bb_usbr_debug("-----------> Closing rdpeusb channel");
			/*sp_context->*/rdpeusb->Close(/*sp_context->*/rdpeusb);
			/*sp_context->*/rdpeusb = NULL;
			// rdpeusb_already_init_flag = 0;
		// }
		// else {
		// 	bb_usbr_debug("-----------> PREVENTING closing rdpeusb channel");
			
		// }
	}

	bb_usbr_debug("end");
	return;
}

static boolean server_peer_post_connect(freerdp_peer* client)
{
	int i;
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
    #ifdef DEBUG_ENABLED
	corrib_syslog (LOG_INFO,"Client %s is activated", client->hostname);
    #endif
	if (client->settings->autologon)
	{
		corrib_syslog(LOG_INFO,"%s autologon using:  %s %s ", __func__,
			client->settings->domain ? client->settings->domain : "",
			client->settings->username);
	}

	bb_usbr_debug("mode: %d", client->connection_mode);


	//we send this irrespective of the client type as the secondary client also needs the null pointer message in order to disable the local cursor
	rdpPointerUpdate* pointer = client->update->pointer;
	pointer->pointer_system.type = SYSPTR_NULL;
	//This should send a system type null for the mouse to disable the local cursor on the Barrow side
	//IFCALL(pointer->PointerSystem, client->context, &pointer->pointer_system);
	//corrib_syslog (LOG_INFO, "%s: client->settings->num_channels %d.\n", __func__, client->settings->num_channels);
	for (i = 0; i < client->settings->num_channels; i++)
	{
		//corrib_syslog (LOG_INFO, "%s: client->settings->channels[%d].joined %d.\n", __func__, i, client->settings->channels[i].joined);
		if (client->settings->channels[i].joined)
		{
			if (strncmp(client->settings->channels[i].name, "rdpsnd", 6) == 0)
			{
				//corrib_syslog(LOG_DEBUG, "%s: <><><><><><><><><>rdpsnd initialization for channel %s.\n", __func__, client->settings->channels[i].name);
				//server_peer_rdpsnd_init(sp_context);
				pthread_mutex_lock(&sp_context->rdpsnd_mutex);
				sp_context->rdpsnd = rdpsnd_server_context_new(sp_context->vcm);
				sp_context->rdpsnd->data = sp_context;
				sp_context->rdpsnd->Activated = server_peer_rdpsnd_activated;
				sp_context->rdpsnd->Initialize(sp_context->rdpsnd);
				pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
				EventChannelReady* audio_channel_ready_event = event_channel_ready_new(client->settings->channels + i); //create an end event to inform the hardware manager that we have sent the command
				eq_push(sp_context->peer_cm_queue, (eqEvent *)audio_channel_ready_event);
			}
            else if (strncmp(client->settings->channels[i].name, "drdynvc", 7) == 0)
            { //ARPM: EXECUTES FIRST
            	bb_usbr_debug("#### Initialization for channel %s ####", client->settings->channels[i].name);
				server_peer_create_rdpeusb_context_and_set_cbs(sp_context, rdpeusb);
			}
		}
	}
	return true;
}


/*
 * This will also be called after a desktop resize event
 */
boolean server_peer_activate(freerdp_peer* client)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	//corrib_syslog(LOG_DEBUG,"%s: client_ID:%s \n",__func__,client->hostname);
	//FIXME, we used to reset the RFX context here, its now done in the hardware manager but we need to understand how this
	//is impacted by a resolution change
	//rfx_context_reset(sp_context->rfx_context); //reset the frame index and reset the frame header indicator, only the first frame should send the header
	sp_context->activated = true;
	return true;
}

//we receive this when the server has received a font_list_pdu
//this marks the end of exchanges from the client
//we can turn video back on at this point, however the hardware manager needs to know this
boolean server_peer_signal(freerdp_peer* client)
{
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	EventServerPeerReady * server_peer_ready_event = event_server_peer_ready_new(client->sockfd); //server peer id
	//corrib_syslog(LOG_DEBUG,"%s:Desktop Resize Complete....\n",__func__);
	client->settings->width = client->settings->connection_resolution[FIRST_HEAD].width;
	client->settings->height = client->settings->connection_resolution[FIRST_HEAD].height;
	server_peer_ready_event->client = client;
	eq_push(sp_context->peer_cm_queue,(eqEvent *)server_peer_ready_event); //tell the connection manager about this
	return true;
}

boolean server_peer_read_output_report(freerdp_peer* client)
{
	boolean result = true;
	//FIXME sh_read_and_process_outputReport(client);
	return(result);
}
//-------------------------------------------------------------



static void server_peer_init(freerdp_peer* client, cmContext* cm_context)
{

	client->context_size = sizeof(serverPeerContext); //this will be used by freerdp_peer_context_new to allocate necessary space for the client context
	client->ContextNew = (psPeerContextNew) server_peer_context_new;
	client->ContextFree = (psPeerContextFree) server_peer_context_free; //this will be invoked from freerdp_peer_context_free
	freerdp_peer_context_new(client,cm_context->peer_cm_queue); //this will force a call to server_peer_context_new through the ContextNew callback, after this client->context is valid
	serverPeerContext * sp_context = (serverPeerContext *)client->context;
	sp_context->client = client;
	//---------------------------------------
	rdpSettings* settings;
	settings = client->settings;


	//corrib_syslog(LOG_INFO,"<<<<<<<< Initialised resolution settings in %s detected width = %d>>>>>>>>>>>>>>>\n",__func__,cm_context->hm_context->videoStatistics.resolution_width_h1);

	if(cm_context->hm_context->head_detected[0] && cm_context->hm_context->head_detected[1])
		settings->num_monitors_detected = 2;
	else
		settings->num_monitors_detected = 1;

	//All of these here are subject to a race with server_peer_post_connect
	//TODO MD - We need to set these are we know what the client is
	//settings->connection_resolution[FIRST_HEAD] = *cm_context->hm_context->connection_resolution[FIRST_HEAD];
	//settings->connection_resolution[SECOND_HEAD] = *cm_context->hm_context->connection_resolution[SECOND_HEAD];
	settings->head_detected[FIRST_HEAD] = cm_context->hm_context->head_detected[FIRST_HEAD];
	settings->head_detected[SECOND_HEAD] = cm_context->hm_context->head_detected[SECOND_HEAD];

	settings->performance_analysis = cm_context->performance_analysis;
	settings->debug_enabled = cm_context->debug_enabled;
	settings->cert_file = xstrdup("/opt/blackbox/shfreerdp/server.crt");
	settings->privatekey_file = xstrdup("/opt/blackbox/shfreerdp/server.key");
	settings->rdp_key_file = xstrdup("/opt/blackbox/shfreerdp/rdp.key");

	settings->nla_security = false;
	settings->rfx_codec = true;
	//Callbacks
	client->PostConnect = server_peer_post_connect;
	client->SignalClientReady = server_peer_signal_client_ready;
	client->SignalMulticastInfo = server_peer_signal_multicast_info;
	client->SignalResChangeComplete = server_peer_signal_res_change_complete;
	client->SignalAccessStatus = server_peer_signal_access_status;
	client->SignalRecoveryRequest = server_peer_signal_recovery_request;
	//client->SendCloudiumMessage = server_peer_send_cloudium_message; //should only be called from the peer

	/*
	 *	USBR Initialization
	 */
	bb_usbr_debug("Callback init: InitialiseUsbrServer CloseUsbrChannel");
	client->InitialiseUsbrServer = server_peer_initialise_rdpeusb_server;
	client->CloseUsbrChannel = server_peer_close_rdpeusb_channel;
	/*****/

	client->Capabilities = server_peer_capabilities;
	client->Activate = server_peer_activate;
	client->Signal = server_peer_signal;
	client->ReadOutputReport = server_peer_read_output_report;
	client->video_channel = -1;
	client->audio_channel = -1;
	client->video_slave_cid = 0;
	client->audio_slave_cid = 0;
	client->last_rtt = 0;
	client->last_mss = 0;

	server_input_register_callbacks(client->input);
	//client->peer_type = PRIMARY_PEER;
	client->connection_state = PEER_CONNECTION_STATE_INIT;
	client->Initialize(client);
	//---------------------------------------
#ifdef MEMORY_ALLOCATION_MONITOR
    char id[255];
    snprintf(id,255,"%s_sp_sp_queue",__func__);
    sp_context->sp_sp_queue = eq_queue_new(id);
    snprintf(id,255,"%s_sp_mon_queue",__func__);
    sp_context->sp_mon_queue = eq_queue_new(id);
#else
    sp_context->sp_sp_queue = eq_queue_new();
    sp_context->sp_mon_queue = eq_queue_new();
#endif
    eq_set_name(sp_context->sp_sp_queue,"sp_sp_queue");
    eq_set_name(sp_context->sp_mon_queue,"sp_mon_queue");
	server_peer_run(sp_context);

}

#if 1

//Main Processing Loop and thread management
//-------------------------------------------
static void server_peer_run(serverPeerContext* server_peer_context)
{
    int main_thread_wait_count = 0;

    if(server_peer_context)
    {

        server_peer_context->main_thread = server_peer_create_thread(server_peer_main_loop,server_peer_context,"main");
#ifdef TWO_THREADS
        //
        // The code logic requires the 'main_loop' to be running before the 'secondary_loop' (BUG-6088)
        // This addition will ensure that this always happens.
        // 
        while((!server_peer_context->main_thread_running) && (main_thread_wait_count < MAX_ML_WAIT_COUNT))
        {
            usleep(200);
            main_thread_wait_count += 1;
        }
        corrib_syslog(LOG_DEBUG,"%s():- Waited (200 * %d (ns))for server_peer_main_thread to start before starting server_peer_secondary_thread.",__func__,main_thread_wait_count);

        if(main_thread_wait_count == MAX_ML_WAIT_COUNT)
        {
            corrib_syslog(LOG_ERR,"%s():- server_peer_main_thread hasn't started within the wait time - continuing in any case...",__func__);
        }

        server_peer_context->secondary_thread = server_peer_create_thread(server_peer_secondary_loop,server_peer_context,"secondary");
#endif
        if(server_peer_context->main_thread == -1 || (server_peer_context->secondary_thread == -1))
        {
            corrib_syslog(LOG_ERR,"Server Peer could not create threads in %s, terminating\n",__func__);
            exit(0);
        }
        else
        {
            pthread_detach(server_peer_context->main_thread);
#ifdef TWO_THREADS
            pthread_detach(server_peer_context->secondary_thread);
#endif
        }
#ifdef TWO_THREADS
        server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_RR,0); //set the video thread as low as possible
        server_peer_set_specific_thread_priority(server_peer_context->secondary_thread,SCHED_FIFO,99); //set the mouse handling as high as possible
#else
        server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_FIFO,99);
#endif
        //server_peer_set_specific_thread_priority(server_peer_context->secondary_thread,SCHED_RR,0); //set the video thread as high as possible
        //server_peer_set_specific_thread_priority(server_peer_context->main_thread,SCHED_FIFO,99); //set the mouse handling as low as possible
    }
    else
        corrib_syslog(LOG_ERR,"Server Context not initialised in %s\n",__func__);

}

static pthread_t server_peer_create_thread( void* func, void* arg,char thread_type[])
{
	pthread_t thread;
	if(pthread_create(&thread, 0, func, arg) != 0)
	{
		perror("pthread:");
		return -1;
	}
	else
	{
		//corrib_syslog(LOG_INFO,"Created %s thread for Server Peer\n",thread_type);
		return thread;
	}
	return 0;
}


/*
 * This needs to be capable of getting all of the associated fds including those supplied by the client peer
 */
static boolean server_peer_get_main_fds(serverPeerContext* sp_context, void** rfds, int* rcount)
{
	freerdp_peer * client = sp_context->client;
	assert(client);
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"server peer has exceeded maximum file descriptors (%d) in %s\n",MAX_FDS, __func__);
		return false;
	}
	//get the queue FD from the connection_manager as we must process those events
	int fd_cm_peer = eq_get_queue_fd(sp_context->cm_peer_queue);
	if(fd_cm_peer < 1)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to get cm_queue file descriptor for queue %p.\n", __func__,sp_context->cm_peer_queue);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(fd_cm_peer);
		(*rcount)++;
	}
	int sp_sp_peer = eq_get_queue_fd(sp_context->sp_sp_queue);
	if (sp_sp_peer < 1)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to get sp_sp_queue file descriptor for queue %p.\n", __func__,sp_context->sp_sp_queue);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(sp_sp_peer);
		(*rcount)++;
	}

	if (/*sp_context->*/rdpeusb)
	{
		/*sp_context->*/rdpeusb->GetFileDescriptors(/*sp_context->*/rdpeusb, rfds, rcount);
	}
pthread_mutex_lock(&sp_context->rdpsnd_mutex);
	if(sp_context->rdpsnd || /*sp_context->*/rdpeusb)
	{
		//printf("%s: Getting Virtual fds.\n", __func__);
		WTSVirtualChannelManagerGetFileDescriptor(sp_context->vcm, rfds, rcount);
	}
pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
#ifndef TWO_THREADS
	if(client->GetFileDescriptor(client, rfds, rcount) != true)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to get FreeRDP file descriptor.\n", __func__);
		return false;
	}

#endif

	return true;
}


/*
 * This needs to be capable of getting all of the associated fds including those supplied by the client peer
 */
static boolean server_peer_get_secondary_fds(serverPeerContext* sp_context, void** rfds, int* rcount)
{
	freerdp_peer * client = sp_context->client;
	assert(client);
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"server peer has exceeded maximum file descriptors (%d) in %s\n",MAX_FDS, __func__);
		return false;
	}
	if (client->GetFileDescriptor(client, rfds, rcount) != true)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to get FreeRDP file descriptor.\n", __func__);
		return false;
	}
	int sp_sp_peer = eq_get_queue_fd(sp_context->sp_sp_queue);
	if (sp_sp_peer < 1)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to get sp_sp_queue file descriptor for queue %p.\n", __func__,sp_context->sp_sp_queue);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(sp_sp_peer);
		(*rcount)++;
	}
	return true;
}



void server_peer_get_timeout_interval(freerdp_peer* client, struct timeval* tv)
{
	if(client != NULL && tv != NULL)
	{
		time_t now = time(NULL);

		if(client->settings->interval_time > now && client->settings->expiration_time > now )
		{
			tv->tv_sec = client->settings->expiration_time <= client->settings->interval_time ?
						 client->settings->expiration_time - now: client->settings->interval_time - now;
			tv->tv_usec = 0;
		}
		else
		{
			memset(tv, 0x00, sizeof(struct timeval));
		}

	}
}


//Clean up any outstanding surface commands before the peer terminates
static void server_peer_deinit(serverPeerContext* sp_context)
{
	int i = 0;
	uint32 usb_commands_found = 0;
	uint32 surface_commands_found = 0;
#ifdef SHARED_MODE_DEBUG
	corrib_syslog(LOG_DEBUG,"%s Looking for events to purge",__func__);
#endif
	if (sp_context && sp_context->cm_peer_queue != NULL ) {
		
		server_peer_release_rdpeusb_context(sp_context,rdpeusb);
		/* Purge queues */
		for ( i = 0; i < sp_context->cm_peer_queue->count; i++ ) {
			eqEvent* event = eq_pop(sp_context->cm_peer_queue);
			switch (event->type) {
				case EQ_EVENT_SURFACE_COMMAND_AVAILABLE:
				{
					EventSurfaceCommandAvailable * surface_command_available_event= (EventSurfaceCommandAvailable *)event;

					EventSurfaceCommandComplete* surface_command_complete_event = event_surface_command_complete_clone_from_available_command(surface_command_available_event);
					surface_command_complete_event->command_id = surface_command_available_event->command_id;
					//		event_surface_command_sent_new(surface_command_available_event->head_id,surface_command_available_event->frame_type,surface_command_available_event->frame_number,surface_command_available_event->cmd); //create an end event to inform the hardware manager that we have sent the command
					eq_push(sp_context->peer_cm_queue, (eqEvent *)surface_command_complete_event);
					surface_commands_found++;
					sp_context->surface_commands_is_use--;
#ifdef SHARED_MODE_DEBUG
					corrib_syslog(LOG_DEBUG,"%s purging EQ_EVENT_SURFACE_COMMAND_COMPLETE in server_peer",__func__);
#endif
					break;
				}
				case EQ_EVENT_USB_COMMAND_AVAILABLE:
				{
					EventUsbCommandAvailable *usb_command_available_event = (EventUsbCommandAvailable *)event;
					usb_command_available_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
					eq_push(sp_context->peer_cm_queue, (eqEvent *)usb_command_available_event); //ARPM: deinit
					usb_commands_found++;
					break;
				}
				case EQ_EVENT_AUDIO_COMMAND_AVAILABLE:
				{
					/*
					 * Jira BUG-2473:
					 * ARPM: If there are audio commands queued to the peer is leaving the session then it is required to
					 * release the memory they are holding. Those commands will die here, I won't send them back to connection
					 * manager.
					 */
					EventAudioCommandAvailable* audio_command_available_event = (EventAudioCommandAvailable *)event;
					event_audio_command_available_free(audio_command_available_event);
					break;
				}
				default:
				{
					corrib_syslog(LOG_DEBUG, "%s(): ### Unknown event found in cm->peer queue [event: %d]. Releasing header. TODO: Release it properly depending on event->type. ###\n", __func__, event->type);
					eq_event_free((eqEvent *)event); //free the event
					break;
				}
			}
			/* ARPM: We shouldn't free events at this stage. They have to be consumed by connection manager */
		}
        #ifdef DEBUG_ENABLED
		corrib_syslog (LOG_INFO, "%s(): Cleaning remaining commands in cm->peer queue. Found %u usb_commands and %u surface_commands", __func__, usb_commands_found, surface_commands_found);
        #endif
	}

}

static void server_peer_set_specific_thread_priority(pthread_t thread_id, int new_policy,int new_priority)
{
	struct sched_param param;
	int existing_policy;
	pthread_attr_t thread_attributes;

	pthread_attr_init(&thread_attributes);
	pthread_attr_getschedpolicy(&thread_attributes, &existing_policy);


	pthread_getschedparam(thread_id, &existing_policy, &param);
	//printf("thread 1: existing policy=%1d, priority=%d\r\n",existing_policy,param.sched_priority);
	param.sched_priority = sched_get_priority_min(existing_policy) +1;
	pthread_setschedparam(thread_id, new_policy, &param);
	pthread_setschedprio(thread_id, new_priority);

	pthread_getschedparam(thread_id, &existing_policy, &param);
	//printf("thread 2: existing policy=%1d, priority=%d\r\n",existing_policy,param.sched_priority);
	pthread_attr_destroy(&thread_attributes);
}


/*
 * The server peer must monitor multiple queues, one from the listner, one from the hardware manager, and one from each peer
 */
static void * server_peer_main_loop(void * arg)
{
	serverPeerContext* sp_context = (serverPeerContext* )arg;

	freerdp_peer * client = sp_context->client;
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[32];
	fd_set rfds_set;
	sp_context->main_thread_running = true;
	int num_set;
	assert(client);
	memset(rfds, 0, sizeof(rfds));
	struct timeval tv = {.tv_sec = DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	sp_context->main_thread_state = RUNNING;
	boolean monitor_keepalives = (client->settings->keepalive.info_flags != 0);

	int testRDPEUSB = 0;
	int testRDPSND = 0;

	if (monitor_keepalives)
	{
		/*
		 * Spawn a separate monitor thread to check for expiry of keep-alive messages
		 * received from the remote peer which would indicate loss of connectivity.
		 *
		 * A separate thread is needed to avoid having these checks delayed by blocking
		 * I/O operations such as a socket send() which can be blocked for up to 15
		 * minutes when a network link goes down.
		 */
		sp_context->monitor_thread = server_peer_create_thread(server_peer_monitor_loop,
								       sp_context, "monitor");
		if (sp_context->monitor_thread == -1)
		{
			corrib_syslog(LOG_ERR, "%s: failed to create monitor thread: %s\n",
				      __func__, strerror(errno));
			goto exit;
		}
		server_peer_set_specific_thread_priority(sp_context->monitor_thread, SCHED_RR, 0);
	}

	//corrib_syslog(LOG_INFO,"Starting server main loop for peer %s in %s\n",sp_context->client->hostname,__func__);

	while(sp_context->main_thread_running)
	{

		rcount = 0;
		server_peer_get_timeout_interval(client, &tv);
		if (server_peer_get_main_fds(sp_context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get server peer file descriptors in %s\n",__func__);
			sp_context->main_thread_running = false;
			sp_context->main_thread_state = STOPPED;
			break;
		}
		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
		{
			sp_context->main_thread_running = false;
			sp_context->main_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			break;
		}
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",  __func__,strerror(errno));
				sp_context->main_thread_running = false;
				sp_context->main_thread_state = STOPPED;
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				break;
			}
		} //everything is as we expected
		else
		{
			int cm_peer_fd = eq_get_queue_fd(sp_context->cm_peer_queue);
#ifndef TWO_THREADS
			if (client->CheckFileDescriptor(client) != true)  //this is the checking of the actual RDP transport 
			{
				corrib_syslog(LOG_INFO,"%s: Failed to check freerdp file descriptor for Peer, terminating peer. The peer may have left the connection\n", __func__);
				/* this can indicate a loss of connection to the client side */
				sp_context->main_thread_running = false;
				sp_context->main_thread_state = STOPPED;
				//okay we are terminating the connection for known or unknown reasons
				//we need to encure there are no outstanding signals that need to be processed before we go
				//search the queue for the relevant events and act on them if required
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				break;
			}
#endif
			if (FD_ISSET(cm_peer_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
				eqEvent* event = eq_pop(sp_context->cm_peer_queue);
				if(event)
				{
					if(sp_context->client->settings->performance_analysis)
					{
						event->receive_time = sh_log_get_mstime();
					}
					switch(event->type)
					{
					case EQ_EVENT_END: //this is only issued by the secondary_peer
						//corrib_syslog(LOG_INFO,"%s: got an end event, terminating.\n",  __func__,strerror(errno));
						sp_context->main_thread_running = false;
						sp_context->main_thread_state = STOPPED;
						//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
						//server_peer_deinit(sp_context);
						eq_push(sp_context->sp_sp_queue, (eqEvent *)event);
						break;
					case EQ_EVENT_ERROR_INFO:
					{
						EventErrorInfo * event_error_info = (EventErrorInfo *)event;
						corrib_syslog(LOG_ERR,"Sending client error info: %d\n",event_error_info->error_code);
						client->SendErrorInfoData(client,event_error_info->error_code);
						break;
					}
					case EQ_EVENT_AUDIO_COMMAND_AVAILABLE:
					{
#if 0
						//corrib_syslog(LOG_DEBUG,"server_peer_main_loop: GOT AUDIO COMMAND\n");
#endif
						EventAudioCommandAvailable *audio_command_available_event = (EventAudioCommandAvailable *)event;
						assert(sp_context);
						pthread_mutex_lock(&sp_context->rdpsnd_mutex);
						if(sp_context->rdpsnd != NULL)
							sp_context->rdpsnd->ProcessCommand(sp_context->rdpsnd, audio_command_available_event->cmd);
						pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
						audio_data_command_free(audio_command_available_event->cmd);
						break;
					}
					case EQ_EVENT_USB_COMMAND_AVAILABLE:
					{
						bb_usbr_debug("sp_context: %p /*sp_context->*/rdpeusb: %p", sp_context, /*sp_context->*/rdpeusb);
						EventUsbCommandAvailable *usb_command_available_event = (EventUsbCommandAvailable *)event;


						if(/*sp_context->*/rdpeusb != NULL)
						{
							if(/*sp_context->*/rdpeusb->ProcessHostCommand(/*sp_context->*/rdpeusb,
																		usb_command_available_event->sequence_data,
																		usb_command_available_event->device_id,
																		usb_command_available_event->cmd))
							{
								/* UnRecoverable Error Processing USB Command - exit connection ... */
								corrib_syslog(LOG_ERR,"Error Processing USB Command ...............in. %s\n",__func__);
								//sp_context->main_thread_running = false;
								//sp_context->main_thread_state = STOPPED;
								//break;
							}
						}
						else
						{
							bb_usbr_debug("/*sp_context->*/rdpeusb is NULL: Releasing USB event");
							usb_command_available_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
							eq_push(sp_context->peer_cm_queue, (eqEvent *)usb_command_available_event);
							event = NULL;
							//usb_command_free(usb_command_available_event->cmd);
						}
						break;
					}
					case EQ_EVENT_DESKTOP_RESIZE:
					{
						EventDesktopResize* event_desktop_resize = (EventDesktopResize*)event;
						rdpUpdate* update = client->update;
						update->DesktopResize(update->context,event_desktop_resize->width,
								event_desktop_resize->height,event_desktop_resize->refresh,event_desktop_resize->sync_loss,
								sp_context->client->video_slave_cid,sp_context->client->video_sequence_number);
						break;
					}
					default:
						corrib_syslog(LOG_ERR,"%s: got an unknown event type:%d.\n",  __func__,event->type);
						break;
					}
					eq_event_free(event); //free the event
				}


			} //cm_peer event
			int sp_sp_fd = eq_get_queue_fd(sp_context->sp_sp_queue);
			if (FD_ISSET(sp_sp_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
				eqEvent* event = eq_pop(sp_context->sp_sp_queue);
				if(sp_context->client->settings->performance_analysis)
				{
					event->receive_time = sh_log_get_mstime();
				}
				if(event)
				{
					switch(event->type)
					{
						case EQ_EVENT_END:
						{
							corrib_syslog(LOG_NOTICE,"%s: got an end event from sp_sp queue, terminating.\n",  __func__);
							sp_context->client->terminating = true;
							sp_context->main_thread_running = false;
							sp_context->main_thread_state = STOPPED;
							//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
							//server_peer_deinit(sp_context);
							eq_event_free(event); //free the event
							break;
						}
						default:
							corrib_syslog(LOG_ERR,"%s: got an unknown event type:%d..\n",  __func__,event->type);
							break;
					}
				}
			} //sp_sp queue

			if (sp_context->main_thread_running) //ARPM: Checking USBR file descriptors
			{
				if (WTSVirtualChannelManagerCheckFileDescriptor(sp_context->vcm))
				{
					pthread_mutex_lock(&sp_context->rdpsnd_mutex);
					if (sp_context->rdpsnd)
					{
						if (!sp_context->rdpsnd->CheckFileDescriptor(sp_context->rdpsnd))
						{
							testRDPSND = -1;
							corrib_syslog(LOG_ERR,"%s DEBUG_ENABLEDs: Failed to read rdpsnd file descriptor.\n", __func__);
							break;
						}
					}
					pthread_mutex_unlock(&sp_context->rdpsnd_mutex);
					if (/*sp_context->*/rdpeusb)
					{
						if (!/*sp_context->*/rdpeusb->CheckFileDescriptors(/*sp_context->*/rdpeusb))
						{
							testRDPEUSB = -1;
							corrib_syslog(LOG_ERR,"%s: Failed to read rdpeusb file descriptor.\n", __func__);
							break;
						}
					}
				}
			}
		}

		if ((tv.tv_sec == 0) && (tv.tv_usec == 0))
		{
			client->settings->interval_time = time(NULL) + DEFAULT_INTERVAL_PERIOD;

			/* Send a new keepalive message periodically to the remote peer to
			 * let them know we're still connected. */
			if (client->activated)
				peer_send_cloudium_pdu(client);
		}
	} //end of while loop

exit:
#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG, "%s: Main Loop terminating: DEV FLAGS - testRDPSND: %d testRDPEUSB: %d\n", __func__, testRDPSND, testRDPEUSB);

	corrib_syslog(LOG_DEBUG,"%s:main_loop terminating: sp_context->main_thread_running =%d",__func__,sp_context->main_thread_running );
#endif
	//we now need to free any resources associated with this client
	//The connection manager created the queues for this peer so it is responsible for purging and destroying the queues
	//The CM needs to know about this event because it may need to inform multicast layers and or multiple unicast connections and in any event remove the peer from the list of active peers

	//if(sp_context->secondary_thread_running)
	//{
	//	//corrib_syslog(LOG_INFO,"%s:requesting secondary thread to complete.",__func__);
	//	EventEnd* event_end = event_end_new();
	//	eq_push(sp_context->sp_sp_queue, (eqEvent *)event_end);
	//	//corrib_syslog(LOG_INFO,"%s:complete pushed.",__func__);
	//	while(sp_context->secondary_thread_running) { printf("."); sleep(1); }
	//}

	if (monitor_keepalives)
	{
        #ifdef DEBUG_ENABLED
		corrib_syslog(LOG_INFO, "%s: requesting monitor thread to complete.", __func__);
        #endif
		EventEnd* event_end = event_end_new();
		eq_push(sp_context->sp_mon_queue, (eqEvent *)event_end);
		pthread_join(sp_context->monitor_thread, NULL);
	}

	bb_usbr_debug("#### server_peer_deinit ####");
	server_peer_deinit(sp_context);


	//TODO RMC, need to add the peer_cide to this event
	EventPeerTerminated* peer_terminated_event =  event_peer_terminated_new(client->sockfd,0); //create an end event to stop the hardware manager
	peer_terminated_event->frame_processing_h1 = sp_context->frame_processing_h1;
	peer_terminated_event->frame_processing_h2 = sp_context->frame_processing_h2;
	eq_push(sp_context->peer_cm_queue, (eqEvent *)peer_terminated_event);

	pthread_exit(NULL);
	return NULL;

}
#endif


static void * server_peer_monitor_loop(void * arg)
{
	serverPeerContext* sp_context = (serverPeerContext* )arg;
	freerdp_peer * client = sp_context->client;

	sp_context->monitor_thread_running = true;
	sp_context->monitor_thread_state = RUNNING;

	while(sp_context->monitor_thread_running)
	{
		int ret;
		fd_set rfds_set;
		int sp_mon_queue_fd;
		struct timeval tv = { .tv_sec = DEFAULT_INTERVAL_PERIOD, .tv_usec = 0 };

		sp_mon_queue_fd = eq_get_queue_fd(sp_context->sp_mon_queue);
		if (sp_mon_queue_fd < 1)
		{
			corrib_syslog(LOG_ERR,"%s: Failed to get sp_mon_queue file descriptor for queue %p.\n",
				      __func__,sp_context->sp_mon_queue);
			break;
		}

		FD_ZERO(&rfds_set);
		FD_SET(sp_mon_queue_fd, &rfds_set);

		// Wait for next interval or an event from the main loop
		ret = select(sp_mon_queue_fd + 1, &rfds_set, NULL, NULL, &tv);
		if (ret == -1)
		{
			/* these are not really errors */
			if (((errno == EAGAIN) ||
			     (errno == EWOULDBLOCK) ||
			     (errno == EINPROGRESS) ||
			     (errno == EINTR))) /* signal occurred */
				continue;

			corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",
				      __func__,strerror(errno));
			break;
		}

		// Check if main loop has sent a request to terminate
		if (FD_ISSET(sp_mon_queue_fd, &rfds_set))
		{
			//read the queue
			eqEvent* event = eq_pop(sp_context->sp_mon_queue);
			if(event)
			{
				switch(event->type)
				{
				case EQ_EVENT_END:
				{
					// corrib_syslog(LOG_NOTICE,"%s: got an end event from server_peer, terminating.\n",
					// 	      __func__);
					eq_event_free(event); //free the event
					goto exit;
				}
				default:
					corrib_syslog(LOG_ERR,"%s: got an unknown event type:%p - %d...\n",
						      __func__,event,event->type);
					break;
				}
			}
		} //sp_mon queue

		time_t now = time(NULL);
		bool link_active = peer_check_link_activity(client);

		if (now >= client->settings->expiration_time)
		{
			/*
			 * Keep-alive expired on RDP channel but if A/V channels are still
			 * receiving traffic we shouldn't abort the connection.
			 */
			if (link_active ||
			    (client->settings->last_active_checkpoint > (now - (2 * DEFAULT_INTERVAL_PERIOD))))
			{
				client->settings->expiration_time = now + DEFAULT_INTERVAL_PERIOD;
				corrib_syslog(LOG_INFO, "%s: audio/video channel activity detected within last %u seconds, postponing keepalive expiry for %u seconds",
					      __func__, (2 * DEFAULT_INTERVAL_PERIOD), DEFAULT_INTERVAL_PERIOD);
			}
			else
			{
				corrib_syslog(LOG_INFO, "%s: keepalive timeout expired and no received audio/video channel activity detected within last %u seconds, terminating connection",
					      __func__, (2 * DEFAULT_INTERVAL_PERIOD));
				corrib_syslog(LOG_ERR, "%s: NETWORK FAILURE (status expiration)\n",__func__);
				/*
				 * Disable the client socket to interrupt any socket read/write operations which may be blocking the main thread.
				 * This will also trigger termination of the main or secondary threads as they call client->CheckFileDescriptor()
				 * on each iteration, which will fail after the socket is disabled here.
				 */
				shutdown(client->sockfd, SHUT_RDWR);
				goto exit;
			}
		}

		if (client->settings->heartbeat_enabled)
			corrib_syslog(LOG_DEBUG,"[P%d]",client->sockfd);
	}

exit:
	sp_context->monitor_thread_running = false;
	sp_context->monitor_thread_state = STOPPED;

	// corrib_syslog(LOG_INFO,"%s: exiting thread", __func__);
	pthread_exit(NULL);
	return NULL;
}


/*
 * The server peer must monitor mouse events in a seperate thread
 */
static void * server_peer_secondary_loop(void * arg)
{
	serverPeerContext* sp_context = (serverPeerContext* )arg;
	freerdp_peer * client = sp_context->client;
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[32];
	fd_set rfds_set;
	int num_set;
	sp_context->secondary_thread_running = true;
	assert(client);
	memset(rfds, 0, sizeof(rfds));
	struct timeval tv = {.tv_sec = DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	sp_context->main_thread_state = RUNNING;
	//corrib_syslog(LOG_INFO,"Starting server secondary loop for peer %s in %s\n",sp_context->client->hostname,__func__);

	while(sp_context->secondary_thread_running)
	{

		rcount = 0;
		server_peer_get_timeout_interval(client, &tv);
		if (server_peer_get_secondary_fds(sp_context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get server peer file descriptors in %s\n",__func__);
			sp_context->secondary_thread_running = false;
			sp_context->secondary_thread_state = STOPPED;
			break;
		}
		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
		{
			sp_context->secondary_thread_running = false;
			sp_context->secondary_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			break;
		}
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",  __func__,strerror(errno));
				sp_context->secondary_thread_running = false;
				sp_context->secondary_thread_state = STOPPED;
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				break;
			}
		} //everything is as we expected
		else
		{
			int sp_sp_fd = eq_get_queue_fd(sp_context->sp_sp_queue);
			if (FD_ISSET(sp_sp_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
				eqEvent* event = eq_pop(sp_context->sp_sp_queue);
				if(sp_context->client->settings->performance_analysis)
				{
					event->receive_time = sh_log_get_mstime();
				}
				if(event)
				{
					switch(event->type)
					{
						case EQ_EVENT_END:
						{
                            #ifdef DEBUG_ENABLED
							corrib_syslog(LOG_NOTICE,"%s: got an end event from server_peer, terminating.\n",  __func__,strerror(errno));
                            #endif
							sp_context->client->terminating = true;
							sp_context->secondary_thread_running = false;
							sp_context->secondary_thread_state = STOPPED;
							//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
							//server_peer_deinit(sp_context);
							eq_event_free(event); //free the event
							break;
						}
						default:
							corrib_syslog(LOG_ERR,"%s: got an unknown event type:%p - %d...\n",  __func__,event,event->type);
							break;
					}

				}

			} //sp_sp queue

			if (client->CheckFileDescriptor(client) != true)  //this is the checking of the actual RDP transport, this is the main exit point when a connection terminates
			{
				corrib_syslog(LOG_INFO,"%s: The Peer appears to have left the connection, cleaning up.\n", __func__);
				/* this can indicate a loss of connection to the client side */
				sp_context->secondary_thread_running = false;
				sp_context->secondary_thread_state = STOPPED;
				//okay we are terminating the connection for known or unknown reasons
				//we need to encure there are no outstanding signals that need to be processed before we go
				//search the queue for the relevant events and act on them if required
				//corrib_syslog(LOG_INFO,"%s:checking queue for outstanding events.",__func__);
				//server_peer_deinit(sp_context);
				#ifdef MARCUS_MEMORYLEAK_FIX
				rdpeusb_server_context_free(/*sp_context->*/rdpeusb);
				#endif
				//tell the main thread about this
				//corrib_syslog(LOG_INFO,"%s:requesting main thread to complete.",__func__);
				//EventEnd* event_end = event_end_new();
				//eq_push(sp_context->sp_sp_queue, (eqEvent *)event_end);
				break;
			}

		}

	} //end of while loop
	//we assume the main thread will now inform the connection manager
	//we now need to free any resources associated with this client
	//The connection manager created the queues for this peer so it is responsible for purging and destroying the queues
	//The CM needs to know about this event because it may need to inform multicast layers and or multiple unicast connections and in any event remove the peer from the list of active peers


	//EventPeerTerminated* peer_terminated_event =  event_peer_terminated_new(client->sockfd); //create an end event to stop the hardware manager
	//eq_push(sp_context->peer_cm_queue, (eqEvent *)peer_terminated_event);
	//perhaps the connection manager needs to free the peer
	/*
		If you get here, then the client has been disconnected.
	*/

	if(sp_context->main_thread_running)
	{
		//corrib_syslog(LOG_INFO,"%s:requesting main thread to complete.",__func__);
		EventEnd* event_end = event_end_new();
        #ifdef DEBUG_ENABLED
		corrib_syslog(LOG_DEBUG, "%s(): ########### sending END event to main loop #############\n", __func__);
        #endif
		eq_push(sp_context->sp_sp_queue, (eqEvent *)event_end);
	}

	//corrib_syslog(LOG_INFO,"%s:peer termination.",__func__);
	//corrib_syslog(LOG_DEBUG,"******* Exiting %s\n",__func__);
	pthread_exit(NULL);
	return NULL;

}
/**
 * This is called via callback from the listner when a client is accepted
 * At the point were this is called the client has not been initialised so we cannot de-reference the context at this point
 */
void server_peer_accepted(cmContext* cm_context, freerdp_peer* client)
{
	corrib_syslog(LOG_DEBUG,"Server Peer initialising\n");

	server_peer_init(client,cm_context);

}
#endif