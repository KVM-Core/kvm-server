#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>
#include <sys/select.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <math.h>
#include <freerdp/hardware_manager.h>
#include <freerdp/core_event.h>
#include "event_processor.h"

static UINT8 saved_led_status = -1;

hwManagerContext*  hw_manager_new()
{
	hwManagerContext *context;

	context = (hwManagerContext *) xnew(hwManagerContext, __func__);
	if (!context)
	{
		corrib_syslog(LOG_ERR, "%s(): ENOM %d\n", __func__, __LINE__);
		return NULL;
	}

	context->capture_context = capture_layer_context_new();
	if (!context->capture_context)
	{
		corrib_syslog(LOG_ERR, "%s(): ERROR %d\n", __func__, __LINE__);
		return NULL;
	}

	context->configured_compression = UNKNOWN_COMPRESSION;

	uint32_t previous_time = sh_log_get_mstime();

	context->ingress_resolution[FIRST_HEAD] = (videoHead_t){0};
	context->ingress_resolution[SECOND_HEAD] = (videoHead_t){0};
	context->optimised_egress_res[FIRST_HEAD] = (videoHead_t){0};
	context->optimised_egress_res[SECOND_HEAD] = (videoHead_t){0};
	context->lossless_egress_res = (videoHead_t){0};

	context->optimised_path_scaled = false;

	context->num_quants = 3;

	context->fpga_reset_complete = false;
	context->quants = (UINT32 *) xmalloc(context->num_quants * 10 * sizeof(UINT32), __func__); //enough space for 3 sets of 10
	if (!context->quants)
	{
		corrib_syslog(LOG_ERR, "%s(): ENOM %d\n", __func__, __LINE__);
		return NULL;
	}

	context->krdm_fd = -1;
	context->keys_held = 0;

//     for(uint8_t loop = 0; loop < 2; loop++)
//     {
//         context->videoStatistics.optimised_stats[loop].min_tiles_per_frame = 510;
//     	context->videoStatistics.optimised_stats[loop].max_tiles_per_frame = 0;
//     	context->videoStatistics.optimised_stats[loop].min_frames_per_second = 60;
//     	context->videoStatistics.optimised_stats[loop].max_frames_per_second = 0;
//         context->videoStatistics.optimised_stats[loop].min_Mbytes_per_second = 1000000;
//         context->videoStatistics.optimised_stats[loop].max_Mbytes_per_second = 0;
// 		//TODO - Redo stats for scaling
// 		context->videoStatistics.optimised_stats[loop].ingress_resolution = &context->ingress_resolution[loop];
// 		context->videoStatistics.optimised_stats[loop].egress_resolution = &context->optimised_egress_res[loop];
// 		context->videoStatistics.optimised_stats[loop].compression = OPTIMISED;
//     }
// 	context->videoStatistics.lossless_stats.min_tiles_per_frame = 510;
// 	context->videoStatistics.lossless_stats.max_tiles_per_frame = 0;
// 	context->videoStatistics.lossless_stats.min_frames_per_second = 60;
// 	context->videoStatistics.lossless_stats.max_frames_per_second = 0;
// 	context->videoStatistics.lossless_stats.min_Mbytes_per_second = 1000000;
// 	context->videoStatistics.lossless_stats.max_Mbytes_per_second = 0;
// 	context->videoStatistics.lossless_stats.ingress_resolution = &context->ingress_resolution[FIRST_HEAD];
// 	context->videoStatistics.lossless_stats.egress_resolution = &context->lossless_egress_res;
// 	context->videoStatistics.lossless_stats.compression = LOSSLESS;

// 	context->videoStatistics.optimised_stats[FIRST_HEAD].previous_time = previous_time;
//     context->videoStatistics.optimised_stats[SECOND_HEAD].previous_time = previous_time;
// 	context->videoStatistics.lossless_stats.previous_time = previous_time;

// 	context->usbStatistics.min_usb_egress_Kbytes_per_second = 100000;
// 	context->usbStatistics.max_usb_egress_Kbytes_per_second = 0;

// 	context->usbStatistics.min_usb_ingress_Kbytes_per_second = 100000;
// 	context->usbStatistics.max_usb_ingress_Kbytes_per_second = 0;

// 	context->UsbaudioStatistics.moving_average_audio = 0;

    // if(hid_interface_init_hid_devices(context) == false)
    // 	corrib_syslog(LOG_ERR,"%s: Failed to initialise HID interfaces\n",__func__); //FIXME, removing this may cause issues

//     if(virtual_interface_init_devices(&context->virtual_interface) == false)
//     	corrib_syslog(LOG_ERR,"%s: Failed to initialise Virtual interfaces\n",__func__);

    context->signal_new_connection = true;
    context->suspend_video_h1 = true; //initially we suspend video until a connection has reached a point where negotiation is complete
    context->suspend_video_h2 = true;
    context->suspend_audio = true;
    context->outputReportAvailable = false;
    context->ab_x = 0;
    context->ab_y = 0;
    context->receiver_head_count = 1;
    context->enable_hm_heartbeats = false;
    context->enable_hid_tracing = false;

    context->hm_ep_queue = eq_queue_new(__func__);
	if (!context->hm_ep_queue) {
		corrib_syslog(LOG_ERR, "%s(): ENOM %d\n", __func__, __LINE__);
		return NULL;
	}

    eq_set_name(context->hm_ep_queue, "hm_ep_queue");

    context->capture_rate = FPGA_FRAME_YUV_CAPTURE_RATE;
    // context->UsbaudioStatistics.previous_time_audio = previous_time;
    // context->analogaudioStatistics.previous_time_audio = previous_time;
    // context->analogaudioStatistics.moving_average_audio = 0;
    // context->usbStatistics.previous_time_usb = previous_time;
    context->dual_head_board = su_is_dual_head();

	//ARPM: undo hardcoded init...
	context->debug_enabled = true;

    return context;
}

void hw_manager_free(hwManagerContext* context)
{
	if (!context)
		return;

// 	//free all dependent structures first
// 	//close the krdm driver
	if(context->krdm_fd)
	{
		close(context->krdm_fd);
	}

	//close hid interfaces
	// hid_interface_deinit(context);
// 	virtual_interface_deinit(&context->virtual_interface);

	eq_queue_free(context->hm_ep_queue, __func__);
	xfree(context->quants, __func__);
	capture_layer_context_free(context->capture_context);
	xfree(context, __func__);
}

void hw_manager_set_queues(hwManagerContext* context, eqEventQueue* hm_cm_queue, eqEventQueue* cm_hm_queue)
{
	//create  event queue FIXME, who should create these, perhaps the owner CM
	context->hm_cm_queue = hm_cm_queue;
	context->cm_hm_queue = cm_hm_queue;
}

//Resolution Detection
//---------------------------------------------------------
void hw_manager_detect_resolution(hwManagerContext* context, int head)
{
    if(capture_layer_read_resolution(context->capture_context, &context->ingress_resolution[head], head, 1))
	{
	 	corrib_syslog(LOG_NOTICE, "Using a detected resolution of %d X %d %dHz on head %d\n",
									context->ingress_resolution[head].width,
        							context->ingress_resolution[head].height,
									context->ingress_resolution[head].refresh,
									head);

#if defined(_EMERALD4K)	
		if( FIRST_HEAD == head )
		{
			//No scaling on lossless currently
			context->lossless_egress_res = context->ingress_resolution[head];
			//Check if we have an option of scaling (First Head only)
	 		capture_layer_read_scaled_resolution(context->capture_context, &context->optimised_egress_res[FIRST_HEAD], &context->optimised_path_scaled);
			if(context->optimised_path_scaled)
			{
				corrib_syslog(LOG_DEBUG,"Using a scaled resolution of %d X %d %dHz on Optimised Path\n", context->optimised_egress_res[FIRST_HEAD].width,
					context->optimised_egress_res[FIRST_HEAD].height, context->optimised_egress_res[FIRST_HEAD].refresh);
			}
			else
			{
				context->optimised_egress_res[head] = context->ingress_resolution[head];
			}
		}
#else
		context->optimised_egress_res[head] = context->ingress_resolution[head];
#endif
	}
	else
	{
		hw_manager_clear_video_head_structs(context, head);
        corrib_syslog(LOG_NOTICE,"No resolution found assuming detached video cable or no output from PC on HEAD %d.\n", head);
	}
}

void hw_manager_reset_and_configure_fpga(hwManagerContext * context, int head)
{
	corrib_syslog(LOG_INFO, "Resetting FPGA Logic for Head %d..........", head);
	capture_layer_yuv_reset_head(context->capture_context, head);
	if (context->head_detected[head])
	{
		hw_manager_detect_resolution(context, head);
		//Below function only configures the optimised FPGA block
		capture_layer_fpga_configure(context->capture_context, context->optimised_egress_res[head], head);
	}
	else
	{
		corrib_syslog(LOG_INFO, "Not configuring capture and encode for Head %d because no sync was detected", head);
	}
}

//TODO do we need this anymore
static void hw_manager_fpga_signal_new_connection(hwManagerContext * 	context, int head)
{
#if !defined(_EMERALD4K)
	if (context->krdm_fd)
	{
		if (FIRST_HEAD == head)
		{
			write(context->krdm_fd, "9", 1); //signals to the krdm driver that a new connection has been established, this will cause the driver to reset frames count indicator
		}
		else
		{
			write(context->krdm_fd, "8", 1); //signals to the krdm driver that a new connection has been established, this will cause the driver to reset frames count indicator
		}
	}
	else
	{
		corrib_syslog(LOG_ERR , "Failed to signal start of new session to krdm driver,krdm is not open\n");
	}
#endif
}

BOOL hw_manager_open_krdm(hwManagerContext* context)
{
	BOOL status;
	int flags;

	status = true;

	if (context->krdm_fd != -1) //if its already open just return
	{
		return status;
	}

	context->krdm_fd = open(VIDEO_ISR_FILE, O_RDWR ); //| O_NONBLOCK

	if (context->krdm_fd < 0)
	{
		corrib_syslog(LOG_ERR,"Failed to open krdm driver at %s\n",VIDEO_ISR_FILE);
		perror("KRDM:");
		status = false;
	}
	else
	{
		flags = fcntl(context->krdm_fd, F_GETFL, 0);
		fcntl(context->krdm_fd, F_SETFL, flags | O_NONBLOCK);
	}
    return status;

}

/*
 * Can also be called from sync loss and res change handlers
 */
BOOL hw_manager_initialise_fpga(hwManagerContext * context, int head, int pass)
{
	//TODO replace this print with something more sensible
	//corrib_syslog(LOG_INFO,"%s: This has resolution changes disabled and is only using super sync detect signal\n",__func__);

	context->head_detected[head] = capture_layer_detect_sync(context->capture_context, head);
	
	if(context->head_detected[head])
	{
		hw_manager_reset_and_configure_fpga(context, head);
		capture_layer_read_dropped_frames(context->capture_context, head, OPTIMISED);
	}

	if(hw_manager_open_krdm(context))
	{
		hw_manager_fpga_signal_new_connection(context, head);
	}

	capture_layer_set_yuv_interrupt_reg(context->capture_context, head);

	return true;
}


//Main Processing Loop and thread management
//-------------------------------------------
void hw_manager_get_timeout_interval(hwManagerContext * context, struct timeval* tv)
{
	if(context != NULL && tv != NULL)
	{
	    tv->tv_sec = HW_DEFAULT_INTERVAL_PERIOD;
	    tv->tv_usec = 0;

	}
}

long long int getEpochTimeMilliseconds (void)
{
	struct timeval tvalEpochTimeUsec = {.tv_sec=0, .tv_usec=0};
	long long int ret;
	if ( gettimeofday(&tvalEpochTimeUsec, NULL) ) {
		corrib_syslog(LOG_DEBUG, "%s().error\n", __func__);
		ret = -1;
	}
	else {
		ret = (((long long int) tvalEpochTimeUsec.tv_sec) * 1000000ll + (long long int) tvalEpochTimeUsec.tv_usec)/1000;
	}
	return ret;
}

/*
 * This needs to be capable of dynamically adding fds from multiple sources, for example peers who will come and go
 */
BOOL hw_manager_get_fds(hwManagerContext  * hm_context, void** rfds, int* rcount)
{
	int index;
	//struct pollfd ufds;
	// virtualInterface* virtual_interface = &hm_context->virtual_interface;
	//get the queue FD for the hardware manager as we must process his events
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"hardware manager has exceeded maximum file descriptors (%d) in %s\n", MAX_FDS, __func__);
		return false;
	}

	int fd_hm_cm = eq_get_queue_fd(hm_context->cm_hm_queue);
	if (fd_hm_cm < 1)
	{
		corrib_syslog(LOG_ERR,"failed to hm_cm queue fd in %s\n",__func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(fd_hm_cm);
		(*rcount)++;
	}

	if ((hm_context->krdm_fd < 1) /*|| (hm_context->fpga_reset_complete == false)*/)
	{
		corrib_syslog(LOG_ERR,"failed to get krdm fd or fpga reset failed in %s\n",__func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(hm_context->krdm_fd);
		(*rcount)++;
	}

#if 0
	if(hm_context->audio_fd >= 0)
	{
		if (hm_context->audio_fd && hm_context->suspend_audio == false)
		{
			rfds[*rcount] = (void*)(long)(hm_context->audio_fd);
			(*rcount)++;
		}
	}

	for(index = 0; index < virtual_interface->size; index++)
	{
		if(virtual_interface->devices[index].fd > 0)
		{
			//corrib_syslog(LOG_DEBUG,"hw_manager_get_fds adding fd %d\n", virtual_interface->devices[index].fd);
			rfds[*rcount] = (void*)(long)(virtual_interface->devices[index].fd);
			(*rcount)++;
		}
		else
		{
			corrib_syslog(LOG_ERR,"failed to get virtual devices fd in %s\n",__func__);
			return false;
	}
	}
#endif

	// ARPM: It adds the kbd handler to the select function fd array
	if ((hm_context->keyb_fd < 1))
	{
		corrib_syslog(LOG_ERR, "%s(): kbd fd is in error state\n", __func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(hm_context->keyb_fd);
		(*rcount)++;
	}

	return true;
}

void hw_manager_process_interval_tasks(hwManagerContext * context)
{
	context->current_interval_time = sh_log_get_mstime();

	if(freerdp_check_file_exists_and_delete("/tmp/ENABLE_HM_HEARTBEATS"))
	{
	   context->enable_hm_heartbeats = true;
       corrib_syslog(LOG_DEBUG,"HM:%s: Heartbeat enabled.\n",  __func__);
	}

	if(context->enable_hm_heartbeats)
	{
           //output any required audit information here
           context->enable_hm_heartbeats = false;
	}

	if(freerdp_check_file_exists_and_delete("/tmp/ENABLE_HID_TRACING"))
	{
		context->enable_hid_tracing = true;
		corrib_syslog(LOG_DEBUG,"HM:%s: HID tracing enabled.\n",  __func__);
	}

	if(freerdp_check_file_exists_and_delete("/tmp/DISABLE_HID_TRACING"))
	{
		context->enable_hid_tracing = false;
		corrib_syslog(LOG_DEBUG,"HM:%s: HID tracing disabled.\n",  __func__);
	}

#ifdef FPGA_RESET_DEBUG
	if(interval_count++ > 10000)
	{
		corrib_syslog(LOG_DEBUG,"Exiting trying to cause FPGA_RESET issue\n");
		exit(0);
	}
#endif
}

void* hw_manager_main_loop(void * arg)
{
	hwManagerContext * 	context = (hwManagerContext *)arg;
	epContext * ep_context = ep_new(context); //FIXME, move this as appropriate into hm_context
	BOOL running = true;
	int num_set;
	int i;
	int fds;
	int max_fds;
	int rcount;
	int index;
	void* rfds[MAX_FDS];
	fd_set rfds_set;
	memset(rfds, 0, sizeof(rfds));
	//virtualDevice* virtual_device;

	struct timeval tv = {.tv_sec = HW_DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	context->main_thread_state = RUNNING;
	assert(context->cm_hm_queue);
	long long int last_processed = getEpochTimeMilliseconds ();
	long long int next_iteration = last_processed + HW_DEFAULT_INTERVAL_PERIOD*1000;
	time_t current_time = time(NULL);
	while(running)
	{
		rcount = 0;
		//corrib_syslog(LOG_DEBUG,"1.0:Time check at %u\n",sh_log_get_mstime());
		hw_manager_get_timeout_interval(context, &tv);
		if (hw_manager_get_fds(context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get hardware manager file descriptors in %s\n",__func__);
			running = false;
			hw_manager_set_exit_state(context,ERROR,"Failed to get FDs\n");
			context->main_thread_state = STOPPED;
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
			running = false;
			context->main_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			hw_manager_set_exit_state(context,ERROR,"max_fds were 0\n");
			break;
		}

		//corrib_syslog(LOG_DEBUG,"HM_BS\n");
		//corrib_syslog(LOG_DEBUG,"A.B:Time check at %u\n",sh_log_get_mstime());
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		//corrib_syslog(LOG_DEBUG,"A.A:Time check at %u\n",sh_log_get_mstime());

		BOOL known_event = false;
		//corrib_syslog(LOG_DEBUG,"HM_AS\n");
		current_time = time(NULL);
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",  __func__,strerror(errno));
				running = false;
				break;
			}
		} //everything is as we expected
		else
		{
			int cm_hm_fd = eq_get_queue_fd(context->cm_hm_queue);
			if (FD_ISSET(cm_hm_fd, &rfds_set))
			{
				// corrib_syslog(LOG_DEBUG,"2.5:Time check at %u\n",sh_log_get_mstime());
				corrib_syslog(LOG_DEBUG,"Got an cm_hm_fd event\n");
				known_event = true;
				//read the queue
				eqEvent* event = eq_pop(context->cm_hm_queue);
				if(context->performance_analysis)
				{
					event->receive_time = sh_log_get_mstime();
				}
				if(event)
				{
					switch(event->type)
					{
					case EQ_EVENT_END:
						corrib_syslog(LOG_NOTICE, "%s: got an end event, terminating.\n",  __func__);
						running = false;
						hw_manager_set_exit_state(context, NORMAL, "Normal exit, no errors\n");
						context->main_thread_state = STOPPED;
						break;
					case EQ_EVENT_MOUSE:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG, "%s: got an EQ_EVENT_MOUSE\n",  __func__);
						if(context->suspend_video_h1 == false)
						{
// Uncomment							hid_interface_process_mouse_event(context,event);
						}
						else
							corrib_syslog(LOG_DEBUG, "%s: Filtering Mouse Events..\n",  __func__);
						break;
					case EQ_EVENT_KEYBOARD:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG, "%s: got an EQ_EVENT_KEYBOARD\n",  __func__);
						if (context->enable_hid_tracing)
							corrib_syslog(LOG_DEBUG, "%s(): Keyboard event received - code: %d flags: 0x%04x\n",
									__func__,
									((EventKeyboard *)event)->code,
									((EventKeyboard *)event)->flags);
						// Uncomment ARPM hid_interface_process_keyboard_event(context,event);
						break;
					case EQ_EVENT_NO_OP:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG, "%s: got an EQ_EVENT_NO_OP\n",  __func__);
						break;
					case EQ_EVENT_SURFACE_COMMAND_SENT:
					{
						corrib_syslog(LOG_DEBUG, "%s: got an EQ_EVENT_SURFACE_COMMAND_SENT\n",  __func__);
						break;
					}
					case EQ_EVENT_CLIENT_SIDE_READY: 
					{
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG, "%s: got an EQ_EVENT_CLIENT_SIDE_READY\n",  __func__);

						if(saved_led_status != -1)
						{
						    EventKeyboardOutputReport* event_keyboard_output_report = event_keyboard_output_report_new(saved_led_status);
						    event_keyboard_output_report->send_time = sh_log_get_mstime();
						    eq_push(context->hm_cm_queue, (eqEvent *)event_keyboard_output_report);
						}
						break;
					}

					default:
						corrib_syslog(LOG_ERR, "%s: got an unknown event type:%d.\n",  __func__, event->type);
						eq_show_event_type(event->type);
						break;
					}
					eq_event_free(event); //free the event
				}

			}

			if(context->krdm_fd >= 0)
			{
				if(FD_ISSET(context->krdm_fd, &rfds_set) && (context->main_thread_state == RUNNING)) //or if
				{

					known_event = true;
					//corrib_syslog(LOG_DEBUG,"Got a krdm event\n");
					if(context->signal_new_connection)
					{
						//rfx_reset_frame_index(ep_context->vp_context->rfx_context);
						context->signal_new_connection = false;
					}
					ep_process_krdm_event(ep_context, context);
				}
			}

			if(FD_ISSET(context->keyb_fd, &rfds_set) && (context->main_thread_state == RUNNING)) // ARPM: Ouput report has been received
			{
				UINT8 led_status = -1;
				known_event = true;

				if (read(context->keyb_fd, &led_status, 1) < 0) {
					corrib_syslog(LOG_DEBUG,"%s(): kbd read error: %d\n", __func__, errno);
				}
				else {
					saved_led_status = led_status;
					EventKeyboardOutputReport* event_keyboard_output_report = event_keyboard_output_report_new(led_status);
					if (event_keyboard_output_report) {
						event_keyboard_output_report->send_time = sh_log_get_mstime();
						eq_push(context->hm_cm_queue, (eqEvent *)event_keyboard_output_report);
					}
				}
			}

			if(((tv.tv_sec == 0) && (tv.tv_usec == 0)) || (next_iteration < getEpochTimeMilliseconds()))
			{
				known_event = true;
				last_processed = getEpochTimeMilliseconds();
				hw_manager_process_interval_tasks(context);
				next_iteration = last_processed + HW_DEFAULT_INTERVAL_PERIOD * 1000;
			}

			if(known_event == false)
				corrib_syslog(LOG_ERR,"%s: got an unknown event from select.\n",  __func__);

		}
		//corrib_syslog(LOG_DEBUG,"4.0:Time check at %u\n",sh_log_get_mstime());
	} //end of while loop
	//ep_free(ep_context, context); //FIXME, causes double free
	pthread_exit(NULL);

	return NULL;
}

static pthread_t hw_manager_create_thread( void* func, void* arg)
{
	pthread_t thread;

	if(pthread_create(&thread, 0, func, arg) != 0)
		corrib_syslog(LOG_ERR, "%s: Failed to create thread for hw_manager\n", __func__);
	else
	{
		return thread;
	}
	return 0;
}

void hw_manager_run(hwManagerContext * hm_context)
{
#ifdef CONNECTION_PROFILING
	char command[255];
    sprintf(command, "/opt/blackbox/time_check.sh shfreerdp hw_setup_start");
    system(command);
#endif

	dal_get_videoquality(hm_context->quants);
	capture_layer_initialise_yuv_block(hm_context->capture_context, hm_context->quants);

	hw_manager_initialise_fpga(hm_context, FIRST_HEAD, PASS1);
	if (hm_context->dual_head_board) 
	{
		hw_manager_initialise_fpga(hm_context, SECOND_HEAD, PASS1);
	}

#ifdef CONNECTION_PROFILING
    sprintf(command, "/opt/blackbox/time_check.sh shfreerdp hw_setup_ends");
    system(command);
#endif

	hm_context->main_thread = hw_manager_create_thread(hw_manager_main_loop, hm_context);
	hm_context->fpga_reset_complete = true;
	sleep(1);
	// Uncomment virtual_interface_reset_devices(&hm_context->virtual_interface);
}

void hw_manager_set_exit_state(hwManagerContext * context,exitState state,const char * message)
{
	context->hm_exit_state = state;
	strncpy(context->exit_info, message, 255);
}

/**
 * @brief Sets the Data structures of the passed in head to default
 * 
 * @param context hw_manager context
 * @param head Desired video head
 */
void hw_manager_clear_video_head_structs(hwManagerContext * context, int head)
{
	context->ingress_resolution[head] = (videoHead_t){0};
	context->optimised_egress_res[head] = (videoHead_t){0};
#if defined(_EMERALD4K)
	if( FIRST_HEAD == head )
	{
		context->lossless_egress_res = (videoHead_t){0};
	}
	context->optimised_path_scaled = false;
#endif
	context->head_detected[head] = false;
}

static const char *compression_type_name[] = { COMPRESSION_ENUMS(AS_STR) };
static const char *connection_type_name[] = { CONNECTION_MODE_ENUMS(AS_STR) };

static void hw_manager_init_capture_subsystem_for_compression(hwManagerContext * hw_context, int head, COMPRESSION_MODE compression_mode)
{
	corrib_syslog(LOG_DEBUG, "%s: Init %s Capture on Head %d\n", __func__, compression_type_name[compression_mode], head);

	/* Clear DF Reset bit */
	capture_layer_data_engine_reset_head_disable(hw_context->capture_context, head, compression_mode);

	/* Rate limit the capture to 0 */
	capture_layer_rate_control_disable(hw_context->capture_context, head, compression_mode);

	/* Start Capture */
	capture_layer_start_capture(hw_context->capture_context, head, compression_mode);
}

void hardware_manager_init_capture_subsystem(hwManagerContext * hw_context, int head, COMPRESSION_MODE cm_compression_mode)
{
	if (MODE_CHECK(cm_compression_mode, LOSSLESS))
	{
		hw_manager_init_capture_subsystem_for_compression(hw_context, head, LOSSLESS);
	}

	if (MODE_CHECK(cm_compression_mode, OPTIMISED))
	{
		hw_manager_init_capture_subsystem_for_compression(hw_context, head, OPTIMISED);
	}
}

static void hw_manager_stop_capture_subsystem_for_compression(hwManagerContext *hw_context, int head, COMPRESSION_MODE compression_mode)
{
	corrib_syslog(LOG_DEBUG, "%s: Stopping Capture on Head %d for a compression type of %s\n", __func__, head, compression_type_name[compression_mode]);

	/* Stop capture */
	capture_layer_stop_capture(hw_context->capture_context, head, compression_mode, true);

	/* Reset Data Flow controller */
	capture_layer_data_engine_reset_head_enable(hw_context->capture_context, head, compression_mode);

	/* Wait for frames in flight and Reset */
	su_avae_enc_video_egress_reset(head, compression_mode);
}

void hardware_manager_stop_capture_subsystem(hwManagerContext *hw_context, int head, COMPRESSION_MODE cm_compression_mode)
{
	if(MODE_CHECK(cm_compression_mode, LOSSLESS)) 
	{
		hw_manager_stop_capture_subsystem_for_compression(hw_context, head, LOSSLESS);
	}

	if(MODE_CHECK(cm_compression_mode, OPTIMISED))
	{
		hw_manager_stop_capture_subsystem_for_compression(hw_context, head, OPTIMISED);
	}
}

#if 0
#include <freerdp/hardware_manager.h>
#include <freerdp/bitops.h>
#include <freerdp/core_event.h>
#include <freerdp/connection_manager.h>
#include "event_processor.h"
#include <freerdp/utils/sh_logger.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/file.h>
#include "virtual_interface.h"
#include "hid_interface.h"
#include "video_packetizer.h"
#include <corrib_logger.h>


//#define CONNECTION_PROFILING

#include <jansson.h>
#include <ulfius.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netdb.h>

#include <freerdp/utils/memory.h>
//#include "statistics_json.h"
#include <restapi.h>
#include <system_utils.h>
#include <system_avae.h>

//#define DEBUG_ENABLED 1
#define MAX_EVENTS_TO_READ 6


#define ENABLE_WEB_SERVICE_REPORTING


#define MAX_WRITES_TO_STATS_FILE 240

static FILE * fd_stats;
static const char *compression_type_name[] = { COMPRESSION_ENUMS(AS_STR) };
static uint8 saved_led_status = -1;

static pthread_t hw_manager_create_thread( void* func, void* arg);
void * hw_manager_main_loop(void * arg);
static void hw_manager_set_exit_state(hwManagerContext * context,exitState state,const char * message);
static boolean hw_manager_get_fds(hwManagerContext  * hm_context, void** rfds, int* rcount);
static void hw_manager_video_stats_calc(hwManagerContext *context, video_head_index_e head, COMPRESSION_MODE compression_mode, const uint32_t peer_count);
static void hw_manager_fpga_signal_new_connection(hwManagerContext * 	context, int head);
static void hw_manager_write_video_stats_to_json( VideoHeadStatistics *video_stats, const COMPRESSION_MODE compression_mode, video_head_index_e head, const uint32_t peer_count);
static void hw_manager_calculate_average_bitrate(VideoHeadStatistics * video_stats);
static void hw_manager_log_video_stats_to_syslog( VideoHeadStatistics * video_head_stat, const COMPRESSION_MODE cm_compression_mode, const video_head_index_e head, const uint64_t avg_rtt, const uint32_t peer_count );
static void hw_manager_stop_capture_subsystem_for_compression(hwManagerContext *hw_context, int head, COMPRESSION_MODE compression_mode);
static void hw_manager_init_capture_subsystem_for_compression(hwManagerContext *hw_context, int head, COMPRESSION_MODE compression_mode);
/*
 * Memory Management
 */



/*
 * Constructor
 */
hwManagerContext *  hw_manager_new()
{
	hwManagerContext * 	context = xnew(hwManagerContext,__func__);
	context->capture_context = capture_layer_context_new();
	context->configured_compression = UNKNOWN_COMPRESSION;
	uint32_t previous_time = sh_log_get_mstime();

	context->ingress_resolution[FIRST_HEAD] = (videoHead_t){0};
	context->ingress_resolution[SECOND_HEAD] = (videoHead_t){0};
	context->optimised_egress_res[FIRST_HEAD] = (videoHead_t){0};
	context->optimised_egress_res[SECOND_HEAD] = (videoHead_t){0};
	context->lossless_egress_res = (videoHead_t){0};

	context->optimised_path_scaled = false;

	context->num_quants = 3;

	context->fpga_reset_complete = false;
	context->quants = (uint32*) xmalloc(context->num_quants * 10 * sizeof(uint32),__func__); //enough space for 3 sets of 10
	context->krdm_fd = -1;
	context->keys_held = 0;

    for(uint8_t loop = 0; loop < 2; loop++)
    {
        context->videoStatistics.optimised_stats[loop].min_tiles_per_frame = 510;
    	context->videoStatistics.optimised_stats[loop].max_tiles_per_frame = 0;
    	context->videoStatistics.optimised_stats[loop].min_frames_per_second = 60;
    	context->videoStatistics.optimised_stats[loop].max_frames_per_second = 0;
        context->videoStatistics.optimised_stats[loop].min_Mbytes_per_second = 1000000;
        context->videoStatistics.optimised_stats[loop].max_Mbytes_per_second = 0;
		//TODO - Redo stats for scaling
		context->videoStatistics.optimised_stats[loop].ingress_resolution = &context->ingress_resolution[loop];
		context->videoStatistics.optimised_stats[loop].egress_resolution = &context->optimised_egress_res[loop];
		context->videoStatistics.optimised_stats[loop].compression = OPTIMISED;
    }
	context->videoStatistics.lossless_stats.min_tiles_per_frame = 510;
	context->videoStatistics.lossless_stats.max_tiles_per_frame = 0;
	context->videoStatistics.lossless_stats.min_frames_per_second = 60;
	context->videoStatistics.lossless_stats.max_frames_per_second = 0;
	context->videoStatistics.lossless_stats.min_Mbytes_per_second = 1000000;
	context->videoStatistics.lossless_stats.max_Mbytes_per_second = 0;
	context->videoStatistics.lossless_stats.ingress_resolution = &context->ingress_resolution[FIRST_HEAD];
	context->videoStatistics.lossless_stats.egress_resolution = &context->lossless_egress_res;
	context->videoStatistics.lossless_stats.compression = LOSSLESS;

	context->videoStatistics.optimised_stats[FIRST_HEAD].previous_time = previous_time;
    context->videoStatistics.optimised_stats[SECOND_HEAD].previous_time = previous_time;
	context->videoStatistics.lossless_stats.previous_time = previous_time;

	context->usbStatistics.min_usb_egress_Kbytes_per_second = 100000;
	context->usbStatistics.max_usb_egress_Kbytes_per_second = 0;

	context->usbStatistics.min_usb_ingress_Kbytes_per_second = 100000;
	context->usbStatistics.max_usb_ingress_Kbytes_per_second = 0;

	context->UsbaudioStatistics.moving_average_audio = 0;

    if(hid_interface_init_hid_devices(context) == false)
    	corrib_syslog(LOG_ERR,"%s: Failed to initialise HID interfaces\n",__func__); //FIXME, removing this may cause issues

    if(virtual_interface_init_devices(&context->virtual_interface) == false)
    	corrib_syslog(LOG_ERR,"%s: Failed to initialise Virtual interfaces\n",__func__);

    context->signal_new_connection = true;
    context->suspend_video_h1 = true; //initially we suspend video until a connection has reached a point where negotiation is complete
    context->suspend_video_h2 = true;
    context->suspend_audio = true;
    context->outputReportAvailable = false;
    context->ab_x = 0;
    context->ab_y = 0;
    context->receiver_head_count = 1;
    context->enable_hm_heartbeats = false;
    context->enable_hid_tracing = false;
#ifdef MEMORY_ALLOCATION_MONITOR
    context->hm_ep_queue = eq_queue_new(__func__);
#else
    context->hm_ep_queue = eq_queue_new();
#endif
    eq_set_name(context->hm_ep_queue,"hm_ep_queue");
    context->capture_rate = FPGA_FRAME_YUV_CAPTURE_RATE;
    context->UsbaudioStatistics.previous_time_audio = previous_time;
    context->analogaudioStatistics.previous_time_audio = previous_time;
    context->analogaudioStatistics.moving_average_audio = 0;
    context->usbStatistics.previous_time_usb = previous_time;

    context->dual_head_board = su_is_dual_head();

    return context;
}


/* Destructor
 *
 */
void hw_manager_free(hwManagerContext * context)
{
	//free all dependent structures first
	//close the krdm driver
	if(context->krdm_fd)
	{
		close(context->krdm_fd);
	}

	//close hid interfaces
	hid_interface_deinit(context);
	virtual_interface_deinit(&context->virtual_interface);
#ifdef MEMORY_ALLOCATION_MONITOR
	eq_queue_free(context->hm_ep_queue,__func__);
#else
	eq_queue_free(context->hm_ep_queue);
#endif
	xfree(context->quants,__func__);
	capture_layer_context_free(context->capture_context);
	xfree(context,__func__);
}

//Resolution Detection
//---------------------------------------------------------
void hw_manager_detect_resolution(hwManagerContext * context, int head)
{
    if( capture_layer_read_resolution(context->capture_context, &context->ingress_resolution[head], head, 1) == true )
	{
	 	corrib_syslog(LOG_NOTICE,"Using a detected resolution of %d X %d %dHz on head %d\n", context->ingress_resolution[head].width,
            context->ingress_resolution[head].height, context->ingress_resolution[head].refresh, head);

#if defined(_EMERALD4K)	
		if( FIRST_HEAD == head )
		{
			//No scaling on lossless currently
			context->lossless_egress_res = context->ingress_resolution[head];
			//Check if we have an option of scaling (First Head only)
	 		capture_layer_read_scaled_resolution(context->capture_context, &context->optimised_egress_res[FIRST_HEAD], &context->optimised_path_scaled);
			if(context->optimised_path_scaled)
			{
				corrib_syslog(LOG_DEBUG,"Using a scaled resolution of %d X %d %dHz on Optimised Path\n", context->optimised_egress_res[FIRST_HEAD].width,
					context->optimised_egress_res[FIRST_HEAD].height, context->optimised_egress_res[FIRST_HEAD].refresh);
			}
			else
			{
				context->optimised_egress_res[head] = context->ingress_resolution[head];
			}
		}
#else
		context->optimised_egress_res[head] = context->ingress_resolution[head];
#endif
		
	}
	else
	{
		hw_manager_clear_video_head_structs(context, head);
        corrib_syslog(LOG_NOTICE,"No resolution found assuming detached video cable or no output from PC on HEAD %d.\n", head);
	}
}

/**
 * @brief Sets the Data structures of the passed in head to default
 * 
 * @param context hw_manager context
 * @param head Desired video head
 */
void hw_manager_clear_video_head_structs(hwManagerContext * context, int head)
{
	context->ingress_resolution[head] = (videoHead_t){0};
	context->optimised_egress_res[head] = (videoHead_t){0};
#if defined(_EMERALD4K)
	if( FIRST_HEAD == head )
	{
		context->lossless_egress_res = (videoHead_t){0};
	}
	context->optimised_path_scaled = false;
#endif
	context->head_detected[head] = false;
}

float hw_manager_get_rolling_average (float avg, float new_sample,int statistic_type)
{
    avg -= avg / 10;
    avg += new_sample / 10;
    return avg;
}

static void hw_manager_calculate_average_bitrate(VideoHeadStatistics * video_stats)
{
	/* Keep tile related stats to 0, we don't wants to expose it*/
	video_stats->total_tiles_changed=0;
	video_stats->total_tile_size = 0;
	video_stats->average_tiles_per_frame = 0;
	video_stats->average_tile_size= 0;

	/* Bandwidth Calculation */
	video_stats->Mbytes_sent = (float)((float)video_stats->total_bytes_processed/1000000);
	video_stats->average_Mbytes_per_second = video_stats->Mbytes_sent;

	if(video_stats->moving_average_video == 0)
	{
		video_stats->moving_average_video = video_stats->average_Mbytes_per_second;
	}
	video_stats->moving_average_video = hw_manager_get_rolling_average(video_stats->moving_average_video, video_stats->average_Mbytes_per_second, 0);

	if(video_stats->average_Mbytes_per_second < video_stats->min_Mbytes_per_second )
		video_stats->min_Mbytes_per_second = video_stats->average_Mbytes_per_second;
	if(video_stats->average_Mbytes_per_second > video_stats->max_Mbytes_per_second )
		video_stats->max_Mbytes_per_second = video_stats->average_Mbytes_per_second;

	video_stats->total_bytes_processed=0;

	/* FPS Calculation */
	video_stats->frames_per_second = (float)((float)video_stats->total_frames_count);

	if(video_stats->frames_per_second < video_stats->min_frames_per_second )
		video_stats->min_frames_per_second = video_stats->frames_per_second;
	if(video_stats->frames_per_second > video_stats->max_frames_per_second )
		video_stats->max_frames_per_second = video_stats->frames_per_second;

	//TODO - Add new data structs
	video_stats->average_frame_size = video_stats->ingress_resolution->width * video_stats->ingress_resolution->height  * 3;

	video_stats->total_frames_count=0;
}

/**
 * @brief Write the video stats to a json struct in /tmp
 * 
 * @param video_stats pointer to stats for video head and compression type
 * @param compression_mode Compression mode of the stats we are logging
 * @param head the head are we logging
 */
static void hw_manager_write_video_stats_to_json( VideoHeadStatistics *video_stats, const COMPRESSION_MODE compression_mode, video_head_index_e head, const uint32_t peer_count)
{
	char buffer[MAX_LOG_ENTRY_SIZE];
	char file[255];
	//TODO - Add new data structs
	sprintf(buffer, " { \"compression\" : \"%s\",\"fps\" : %3.0f,\"dropped\" : %d,\"dropped_total\" : %u,\"Mbps\" : %2.2f,\"AvgMbps\" : %2.2f,\"egress_resolution\" : \"%dx%d@%dHz\",\"ingress_resolution\" :  \"%dx%d@%dHz\", \"peer_count\" : %d},",
		compression_type_name[compression_mode],
		video_stats->frames_per_second, 
		video_stats->dropped_frames, 
		video_stats->total_dropped_frames, 
		(video_stats->average_Mbytes_per_second * 8), 
		(video_stats->moving_average_video * 8),
		video_stats->egress_resolution->width,
		video_stats->egress_resolution->height,
		video_stats->egress_resolution->refresh,
		video_stats->ingress_resolution->width,
		video_stats->ingress_resolution->height,
		video_stats->ingress_resolution->refresh,
		peer_count);

	sprintf(file,"/tmp/video_stats_%u.json", head);
	su_add_and_rotate(buffer,file);
}

/*
 * Can also be called from sync loss and res change handlers
 */
boolean hw_manager_initialise_fpga(hwManagerContext * context, int head, int pass)
{
	//TODO replace this print with something more sensible
	//corrib_syslog(LOG_INFO,"%s: This has resolution changes disabled and is only using super sync detect signal\n",__func__);
	
	context->head_detected[head] = capture_layer_detect_sync(context->capture_context, head);
	
	if(context->head_detected[head])
	{
		hw_manager_reset_and_configure_fpga(context, head);
		capture_layer_read_dropped_frames(context->capture_context, head, OPTIMISED);
	}
	if(hw_manager_open_krdm(context))
	{
		hw_manager_fpga_signal_new_connection(context, head);
	}
	capture_layer_set_yuv_interrupt_reg(context->capture_context, head);

	return true;
}

void hw_manager_reset_and_configure_fpga(hwManagerContext * context, int head)
{
	corrib_syslog(LOG_INFO, "Resetting FPGA Logic for Head %d..........", head);
	capture_layer_yuv_reset_head(context->capture_context, head);

	if (context->head_detected[head])
	{
		hw_manager_detect_resolution(context, head);
		//Below function only configures the optimised FPGA block
		capture_layer_fpga_configure(context->capture_context, context->optimised_egress_res[head], head);
	}
	else
	{
		corrib_syslog(LOG_INFO, "Not configuring capture and encode for Head %d because no sync was detected", head);
	}
}

void hw_manager_get_connection_resolution( hwManagerContext * context, const COMPRESSION_MODE compression, videoHead_t * connection_res, const video_head_index_e head )
{
	if( MODE_CHECK(compression, LOSSLESS) )
	{
		*connection_res = context->lossless_egress_res;
	}
	if( MODE_CHECK(compression, OPTIMISED) )
	{
		*connection_res = context->optimised_egress_res[head];
	}
	corrib_syslog(LOG_DEBUG,"%s: Connection Res %d X %d %dHz on head %d\n", __func__, connection_res->width,
           	connection_res->height, connection_res->refresh, head);
}

//TODO do we need this anymore
static void hw_manager_fpga_signal_new_connection(hwManagerContext * 	context, int head)
{
#if !defined(_EMERALD4K)
	if(context->krdm_fd)
	{
		if(FIRST_HEAD == head)
		{
			write(context->krdm_fd, "9", 1); //signals to the krdm driver that a new connection has been established, this will cause the driver to reset frames count indicator
		}
		else
		{
			write(context->krdm_fd, "8", 1); //signals to the krdm driver that a new connection has been established, this will cause the driver to reset frames count indicator
		}
	}
	else
	{
		corrib_syslog(LOG_ERR , "Failed to signal start of new session to krdm driver,krdm is not open\n");
	}
#endif
}

//used for test purposes and debug only
void hw_manager_test_fpga_reset(hwManagerContext * hm_context)
{
	boolean run = true;
	int count=1;
	while(run)
	{
		if(hw_manager_initialise_fpga(hm_context,FIRST_HEAD,PASS1) == false)
		{
			corrib_syslog(LOG_ERR , "Initialising FPGA for head 1 in %s failed, trying again.................\n",__func__);
			run = false;
		}
		else
		{
			corrib_syslog(LOG_DEBUG,"Successful reset of FPGA:%d\n",count++);
		}
	}
}


//generates a normally distributed Random Number
static unsigned int random_in_range (unsigned int min, unsigned int max)
{
	int base_random = rand(); /* in [0, RAND_MAX] */
	if (RAND_MAX == base_random) 
	if (RAND_MAX == base_random) 
	{
		return random_in_range(min, max);
	}
	/* now guaranteed to be in [0, RAND_MAX) */
	int range       = max - min;
	int remainder   = RAND_MAX % range;
	int bucket      = RAND_MAX / range;
	/* There are range buckets, plus one smaller interval
	within remainder of RAND_MAX */
	if (base_random < RAND_MAX - remainder) 
	{
		return min + base_random/bucket;
	} 
	else 
	{
		return random_in_range (min, max);
	}
}

//Main Processing Loop and thread management
//-------------------------------------------

void hw_manager_run(hwManagerContext * hm_context)
{
#ifdef CONNECTION_PROFILING
	char command[255];
    sprintf(command, "/opt/blackbox/time_check.sh shfreerdp hw_setup_start");
    system(command);
#endif


	dal_get_videoquality(hm_context->quants);
	capture_layer_initialise_yuv_block(hm_context->capture_context, hm_context->quants);

	hw_manager_initialise_fpga(hm_context,FIRST_HEAD,PASS1);
	if (hm_context->dual_head_board) 
	{
		hw_manager_initialise_fpga(hm_context,SECOND_HEAD,PASS1);
	}

#ifdef CONNECTION_PROFILING
    sprintf(command, "/opt/blackbox/time_check.sh shfreerdp hw_setup_ends");
    system(command);
#endif

	hm_context->main_thread = hw_manager_create_thread(hw_manager_main_loop,hm_context);
	hm_context->fpga_reset_complete = true;
	sleep(1);
	virtual_interface_reset_devices(&hm_context->virtual_interface);
}



boolean hw_manager_open_krdm(hwManagerContext * context)
{
	boolean status = true;
	int flags;
	if(context->krdm_fd != -1) //if its already open just return
	{
		return status;
	}
	context->krdm_fd = open(VIDEO_ISR_FILE, O_RDWR ); //| O_NONBLOCK


	if (context->krdm_fd < 0)
	{
		corrib_syslog(LOG_ERR,"Failed to open krdm driver at %s\n",VIDEO_ISR_FILE);
		perror("KRDM:");
		status = false;
	}
	else
	{
		flags = fcntl(context->krdm_fd, F_GETFL, 0);
		fcntl(context->krdm_fd, F_SETFL, flags | O_NONBLOCK);
	}
    return status;

}


/* this can be expanded to specify individual media like audio or video
 * THis should ONLY be called from the main_loop of the connection manager
 * Calling it from anywhere else requires a consideration of locking*/
void hw_manager_set_media_suspend_state(hwManagerContext * context, hwMediaState state, boolean value)
{
	if(context)
	{
		switch(state)
		{
		case HEAD_ONE:
			//corrib_syslog(LOG_DEBUG,"****** Media suspended state HEAD_ONE = %d\n",value);
			context->suspend_video_h1 = value;
			break;
		case HEAD_TWO:
			//corrib_syslog(LOG_DEBUG,"****** Media suspended state HEAD_TWO = %d\n",value);
			context->suspend_video_h2 = value;
			break;
		case AUDIO:
			//corrib_syslog(LOG_DEBUG,"****** Media suspended state AUDIO = %d\n",value);
			context->suspend_audio = value;
			break;
		case USB:
			//corrib_syslog(LOG_DEBUG,"****** Media suspended state USB = %d\n",value);
			context->suspend_virtual = value;
			break;
		default:
			corrib_syslog(LOG_DEBUG,"****** Undefined Media suspended state %d\n",state);
			break;
		}
	}
}

static pthread_t hw_manager_create_thread( void* func, void* arg)
{
	pthread_t thread;
	if(pthread_create(&thread, 0, func, arg) != 0)
		corrib_syslog(LOG_ERR, "%s: Failed to create thread for hw_manager\n",__func__);
	else
	{

		return thread;
	}
	return 0;
}

void hw_manager_reset_fpga_logic(hwManagerContext  * context)
{
	hw_manager_reset_and_configure_fpga(context, FIRST_HEAD);
	if (context->dual_head_board) 
	{
		hw_manager_reset_and_configure_fpga(context, SECOND_HEAD);
	}
}

/*
 * This needs to be capable of dynamically adding fds from multiple sources, for example peers who will come and go
 */
static boolean hw_manager_get_fds(hwManagerContext  * hm_context, void** rfds, int* rcount)
{
	int index;
	//struct pollfd ufds;
	virtualInterface* virtual_interface = &hm_context->virtual_interface;
	//get the queue FD for the hardware manager as we must process his events
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"hardware manager has exceeded maximum file descriptors (%d) in %s\n", MAX_FDS, __func__);
		return false;
	}
	int fd_hm_cm = eq_get_queue_fd(hm_context->cm_hm_queue);
	if (fd_hm_cm < 1)
	{
		corrib_syslog(LOG_ERR,"failed tohm_cm queue fd in %s\n",__func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(fd_hm_cm);
		(*rcount)++;
	}
	if ((hm_context->krdm_fd < 1) /*|| (hm_context->fpga_reset_complete == false)*/)
	{
		corrib_syslog(LOG_ERR,"failed to get krdm fd or fpga reset failed in %s\n",__func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(hm_context->krdm_fd);
		(*rcount)++;
	}
#if 1
	if(hm_context->audio_fd >= 0)
	{
		if (hm_context->audio_fd && hm_context->suspend_audio == false)
		{
			rfds[*rcount] = (void*)(long)(hm_context->audio_fd);
			(*rcount)++;
		}
	}

	for(index = 0; index < virtual_interface->size; index++)
	{
		if(virtual_interface->devices[index].fd > 0)
		{
			//corrib_syslog(LOG_DEBUG,"hw_manager_get_fds adding fd %d\n", virtual_interface->devices[index].fd);
			rfds[*rcount] = (void*)(long)(virtual_interface->devices[index].fd);
			(*rcount)++;
		}
		else
		{
			corrib_syslog(LOG_ERR,"failed to get virtual devices fd in %s\n",__func__);
			return false;
	}
	}
#endif
	// ARPM: It adds the kbd handler to the select function fd array
	if ((hm_context->keyb_fd < 1))
	{
		corrib_syslog(LOG_ERR,"%s(): kbd fd is in error state\n", __func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(hm_context->keyb_fd);
		(*rcount)++;
	}
/*
	for (i = 0; i < listener->num_sockfds; i++)
	{
		rfds[*rcount] = (void*)(long)(listener->sockfds[i]);
		(*rcount)++;
	}
*/
	return true;
}


void hw_manager_flush_pressed_keys(hwManagerContext * context) 
{
	hid_interface_flush_pressed_keys(context);
	hid_interface_flush_mouse_buttons(context);
}

//#define FPGA_RESET_DEBUG
#ifdef FPGA_RESET_DEBUG
	static interval_count =0;
#endif
void hw_manager_process_interval_tasks(hwManagerContext * context)
{
	context->current_interval_time = sh_log_get_mstime();
	if(freerdp_check_file_exists_and_delete("/tmp/ENABLE_HM_HEARTBEATS"))
	{
	   context->enable_hm_heartbeats = true;
       corrib_syslog(LOG_DEBUG,"HM:%s: Heartbeat enabled.\n",  __func__);
	}

	if(context->enable_hm_heartbeats)
	{
           //output any required audit information here
           context->enable_hm_heartbeats = false;

	}

	if(freerdp_check_file_exists_and_delete("/tmp/ENABLE_HID_TRACING"))
	{
		context->enable_hid_tracing = true;
		corrib_syslog(LOG_DEBUG,"HM:%s: HID tracing enabled.\n",  __func__);
	}

	if(freerdp_check_file_exists_and_delete("/tmp/DISABLE_HID_TRACING"))
	{
		context->enable_hid_tracing = false;
		corrib_syslog(LOG_DEBUG,"HM:%s: HID tracing disabled.\n",  __func__);
	}

#ifdef FPGA_RESET_DEBUG
	if(interval_count++ > 10000)
	{
		corrib_syslog(LOG_DEBUG,"Exiting trying to cause FPGA_RESET issue\n");
		exit(0);
	}
#endif

}

#if 1
void hw_manager_get_timeout_interval(hwManagerContext * context, struct timeval* tv)
{
	if(context != NULL && tv != NULL)
	{
	    tv->tv_sec = HW_DEFAULT_INTERVAL_PERIOD;
	    tv->tv_usec = 0;

	}
}

#endif


long long int getEpochTimeMilliseconds (void)
{
	struct timeval tvalEpochTimeUsec = {.tv_sec=0, .tv_usec=0};
	long long int ret;
	if ( gettimeofday(&tvalEpochTimeUsec, NULL) ) {
		corrib_syslog(LOG_DEBUG, "%s().error\n", __func__);
		ret = -1;
	}
	else {
		ret = (((long long int) tvalEpochTimeUsec.tv_sec) * 1000000ll + (long long int) tvalEpochTimeUsec.tv_usec)/1000;
	}
	return ret;
}


void * hw_manager_main_loop(void * arg)
{
	hwManagerContext * 	context = (hwManagerContext *)arg;
	epContext * ep_context = ep_new(context); //FIXME, move this as appropriate into hm_context
	boolean running = true;
	int num_set;
	int i;
	int fds;
	int max_fds;
	int rcount;
	int index;
	void* rfds[MAX_FDS];
	fd_set rfds_set;
	memset(rfds, 0, sizeof(rfds));
	//virtualDevice* virtual_device;

	struct timeval tv = {.tv_sec = HW_DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	context->main_thread_state = RUNNING;
	assert(context->cm_hm_queue);
	long long int last_processed = getEpochTimeMilliseconds ();
	long long int next_iteration = last_processed + HW_DEFAULT_INTERVAL_PERIOD*1000;
	time_t current_time = time(NULL);
	while(running)
	{
		rcount = 0;
		//corrib_syslog(LOG_DEBUG,"1.0:Time check at %u\n",sh_log_get_mstime());
		hw_manager_get_timeout_interval(context, &tv);
		if (hw_manager_get_fds(context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get hardware manager file descriptors in %s\n",__func__);
			running = false;
			hw_manager_set_exit_state(context,ERROR,"Failed to get FDs\n");
			context->main_thread_state = STOPPED;
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
			running = false;
			context->main_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			hw_manager_set_exit_state(context,ERROR,"max_fds were 0\n");
			break;
		}
		//corrib_syslog(LOG_DEBUG,"HM_BS\n");
		//corrib_syslog(LOG_DEBUG,"A.B:Time check at %u\n",sh_log_get_mstime());
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		//corrib_syslog(LOG_DEBUG,"A.A:Time check at %u\n",sh_log_get_mstime());
		boolean known_event = false;
		//corrib_syslog(LOG_DEBUG,"HM_AS\n");
		current_time=time(NULL);
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"%s: select failed on error: %s.\n",  __func__,strerror(errno));
				running = false;
				break;
			}
		} //everything is as we expected
		else
		{
			//corrib_syslog(LOG_DEBUG,"2.2:Time check at %u  num_set=%d\n",sh_log_get_mstime(),num_set);

			int cm_hm_fd = eq_get_queue_fd(context->cm_hm_queue);

			if (FD_ISSET(cm_hm_fd, &rfds_set))
			{
				//corrib_syslog(LOG_DEBUG,"2.5:Time check at %u\n",sh_log_get_mstime());
				//corrib_syslog(LOG_DEBUG,"Got an cm_hm_fd event\n");
				known_event = true;
				//read the queue
				eqEvent* event = eq_pop(context->cm_hm_queue);
				if(context->performance_analysis)
				{
					event->receive_time = sh_log_get_mstime();
				}
				if(event)
				{
					switch(event->type)
					{
					case EQ_EVENT_END:
						corrib_syslog(LOG_NOTICE,"%s: got an end event, terminating.\n",  __func__);
						running = false;
						hw_manager_set_exit_state(context,NORMAL,"Normal exit, no errors\n");
						context->main_thread_state = STOPPED;
						break;
					case EQ_EVENT_MOUSE:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"%s: got an EQ_EVENT_MOUSE\n",  __func__);
						if(context->suspend_video_h1 == false)
							hid_interface_process_mouse_event(context,event);
						else
							corrib_syslog(LOG_DEBUG,"%s: Filtering Mouse Events..\n",  __func__);
						break;
					case EQ_EVENT_KEYBOARD:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"%s: got an EQ_EVENT_KEYBOARD\n",  __func__);
						if (context->enable_hid_tracing)
							corrib_syslog(LOG_DEBUG, "%s(): Keyboard event received - code: %d flags: 0x%04x\n",
									__func__,
									((EventKeyboard *)event)->code,
									((EventKeyboard *)event)->flags);
						hid_interface_process_keyboard_event(context,event);
						break;
					case EQ_EVENT_NO_OP:
						if(context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"%s: got an EQ_EVENT_NO_OP\n",  __func__);
						break;
					case EQ_EVENT_SURFACE_COMMAND_SENT:
					{
						break;
					}
					case EQ_EVENT_USB_COMMAND_AVAILABLE:
						//corrib_syslog(LOG_DEBUG,"3.5:Time check at %u\n",sh_log_get_mstime());
					{
						//corrib_syslog(LOG_DEBUG,"3.4:Time check at %u\n",sh_log_get_mstime());
						EventUsbCommandAvailable* usb_event = (EventUsbCommandAvailable*)event;
						usb_event->receive_time = sh_log_get_mstime();
#ifdef DEBUG_ENABLED
						corrib_syslog(LOG_DEBUG,"EQ_EVENT_USB_COMMAND_AVAILABLE device_id %d\n", usb_event->device_id);
#endif
						//corrib_syslog(LOG_DEBUG,"3.7:Time check at %u\n",sh_log_get_mstime());
						virtual_interface_process_event(ep_context->up_context, context, event);
						//corrib_syslog(LOG_DEBUG,"3.6:Time check at %u\n",sh_log_get_mstime());
						break;
					}
					case EQ_EVENT_CLIENT_SIDE_READY: 
					{
						if(saved_led_status != -1)
						{
						    EventKeyboardOutputReport* event_keyboard_output_report = event_keyboard_output_report_new(saved_led_status);
						    event_keyboard_output_report->send_time = sh_log_get_mstime();
						    eq_push(context->hm_cm_queue, (eqEvent *)event_keyboard_output_report);
						}
						break;
					}

					default:
						corrib_syslog(LOG_ERR,"%s: got an unknown event type:%d.\n",  __func__,event->type);
						eq_show_event_type(event->type);
					break;
					}
					eq_event_free(event); //free the event
				}

			}
			//corrib_syslog(LOG_DEBUG,"4.0:Time check at %u\n",sh_log_get_mstime());
			//corrib_syslog(LOG_DEBUG,"%s:2.7:Time check at %u\n",__func__,sh_log_get_mstime());
			if(context->krdm_fd >= 0)
			{
				if(FD_ISSET(context->krdm_fd, &rfds_set) && (context->main_thread_state == RUNNING)) //or if
				{

					known_event = true;
					//corrib_syslog(LOG_DEBUG,"Got a krdm event\n");
					if(context->signal_new_connection)
					{
						//rfx_reset_frame_index(ep_context->vp_context->rfx_context);
						context->signal_new_connection = false;
					}
					ep_process_krdm_event(ep_context,context);

				}
			}
			//corrib_syslog(LOG_DEBUG,"2.8:Time check at %u\n",sh_log_get_mstime());
			//corrib_syslog(LOG_DEBUG,"4.5:Time check at %u\n",sh_log_get_mstime());
			if(context->audio_fd >= 0)
			{
				if(FD_ISSET(context->audio_fd, &rfds_set) && (context->main_thread_state == RUNNING)) //or if
				{
					//corrib_syslog(LOG_DEBUG,"2.9:Time check at %u\n",sh_log_get_mstime());
					known_event = true;
					//corrib_syslog(LOG_DEBUG,"Got an audio event\n");
					ep_process_audio_event(ep_context,context);
					//corrib_syslog(LOG_DEBUG,"3.0:Time check at %u\n",sh_log_get_mstime());
				}
			}
			//corrib_syslog(LOG_DEBUG,"5. :Time check at %u\n",sh_log_get_mstime());
			for(index = 0; index < context->virtual_interface.size; index++)
			{
				if(FD_ISSET(context->virtual_interface.devices[index].fd, &rfds_set) && (context->main_thread_state == RUNNING))
				{
					//corrib_syslog(LOG_DEBUG,"3.1:Time check at %u\n",sh_log_get_mstime());
					known_event = true;
					ep_process_virtual_event(ep_context,context, &context->virtual_interface.devices[index]);
					//corrib_syslog(LOG_DEBUG,"3.2:Time check at %u\n",sh_log_get_mstime());
				}
			}
			if(FD_ISSET(context->keyb_fd, &rfds_set) && (context->main_thread_state == RUNNING)) // ARPM: Ouput report has been received
			{
				uint8 led_status = -1;
				known_event = true;

				if (read(context->keyb_fd, &led_status, 1) < 0) {
					corrib_syslog(LOG_DEBUG,"%s(): kbd read error: %d\n", __func__, errno);
				}
				else {
					saved_led_status = led_status;
					EventKeyboardOutputReport* event_keyboard_output_report = event_keyboard_output_report_new(led_status);
					if (event_keyboard_output_report) {
						event_keyboard_output_report->send_time = sh_log_get_mstime();
						eq_push(context->hm_cm_queue, (eqEvent *)event_keyboard_output_report);
					}
				}
			}

			if(((tv.tv_sec == 0) && (tv.tv_usec == 0)) || (next_iteration < getEpochTimeMilliseconds()) )
			{
				known_event = true;
				last_processed = getEpochTimeMilliseconds();
				hw_manager_process_interval_tasks(context);
				next_iteration = last_processed + HW_DEFAULT_INTERVAL_PERIOD * 1000;
			}
			if(known_event == false)
				corrib_syslog(LOG_ERR,"%s: got an unknown event from select.\n",  __func__);

		}

		//corrib_syslog(LOG_DEBUG,"4.0:Time check at %u\n",sh_log_get_mstime());
	} //end of while loop
	//ep_free(ep_context, context); //FIXME, causes double free
	pthread_exit(NULL);

	return NULL;
}


//Access Functions
//---------------------------------------------
void hw_manager_set_queues(hwManagerContext * context,eqEventQueue* hm_cm_queue,eqEventQueue* cm_hm_queue)
{
	//create  event queue FIXME, who should create these, perhaps the owner CM
	context->hm_cm_queue = hm_cm_queue;
	context->cm_hm_queue = cm_hm_queue;
}

static void hw_manager_set_exit_state(hwManagerContext * context,exitState state,const char * message)
{
	context->hm_exit_state = state;
	strncpy(context->exit_info,message,255);

}


void hw_manager_enable_performance_analysis(hwManagerContext * hm_context)
{
	hm_context->performance_analysis = true;
}

void hw_manager_enable_debug(hwManagerContext * hm_context)
{
	hm_context->debug_enabled = true;
}

void hw_manager_check_for_capture_rate_config(hwManagerContext * context)
{
	//we look for a file in /usr/local containing capture rate values
	//if we find one we use the values supplied, if we do not we use the default values
	//the file should contain the values in the following format
	FILE * file;
	file = fopen("/usr/local/capture_rate.cfg", "r");
	if (file)
	{
		corrib_syslog(LOG_NOTICE,"Using a local capture rate configuration at %s\n","/usr/local/capture_rate.cfg");
		fscanf(file, "%u",&context->capture_rate);
		corrib_syslog(LOG_NOTICE,"Capture Rate is :%u\n",context->capture_rate);
		fclose(file);
	}
	else
		corrib_syslog(LOG_NOTICE,"No local capture rate values found using defaults\n");

}

//FIXME, we need to understand when this needs to be done now in a multi connection scenario as new connections do
// not necessarily reset the statistics
void hw_manager_signal_new_connection(hwManagerContext * context)
{
	corrib_syslog(LOG_DEBUG,"11111_%s\n",__func__);
	/* Tell the krdm ISR that a new connection is being established */
	if(context->krdm_fd)
	{
		//printf("Resetting capture with 9\n");
		write(context->krdm_fd, "9", 1); //signals to the krdm driver that a new connection has been established, this will cause the driver to reset frames count indicator
	}
	else
		printf("Error: Failed to signal start of new session to krdm driver\n");

}


void hw_manager_video_stats(hwManagerContext *context, const uint32_t optimised_peers, const uint32_t lossless_peers)
{
#if defined(_EMERALD4K)
	//TODO MD - May need to change the OPTIMISED HEAD reference for dual stream
	if( MODE_CHECK(context->configured_compression, LOSSLESS) )
	{
		hw_manager_video_stats_calc(context, FIRST_HEAD, LOSSLESS, lossless_peers);
	}
	if( MODE_CHECK(context->configured_compression, OPTIMISED) )
	{
		hw_manager_video_stats_calc(context, FIRST_HEAD, OPTIMISED, optimised_peers);
	}
#else
	hw_manager_video_stats_calc(context, FIRST_HEAD, OPTIMISED, optimised_peers);
	if (context->dual_head_board)
	{
		hw_manager_video_stats_calc(context, SECOND_HEAD, OPTIMISED, optimised_peers);
	}
#endif
}

static void hw_manager_video_stats_calc(hwManagerContext *context, video_head_index_e head, COMPRESSION_MODE compression_mode, const uint32_t peer_count)
{
	uint32_t current_time;
	uint32_t elapsed_sec;
	uint64_t total_bytes_sent;
	uint32_t frames_per_second_interval;
	VideoHeadStatistics * video_head_stats;
	COMPRESSION_MODE stat_compression;
	//On 4k we will need to redirect the head to the second_head for optimised connections
	video_head_index_e aae_video_head = head;

	if(LOSSLESS == compression_mode)
	{
		video_head_stats = &context->videoStatistics.lossless_stats;
		stat_compression = LOSSLESS;
	}
	else
	{
		video_head_stats = &context->videoStatistics.optimised_stats[head];
		stat_compression = OPTIMISED;
#if defined(_EMERALD4K)
		aae_video_head = SECOND_HEAD;
#endif
	}

	current_time = sh_log_get_mstime();
	elapsed_sec = (current_time - video_head_stats->previous_time) / 1000;
	frames_per_second_interval = su_get_avae_fps(aae_video_head);
	total_bytes_sent = su_get_avae_bandwidth(aae_video_head);

	//Dropped Frames
	video_head_stats->total_dropped_frames = capture_layer_read_dropped_frames(context->capture_context, head, stat_compression);
	video_head_stats->dropped_frames = video_head_stats->total_dropped_frames / elapsed_sec;
	//If we are in YUV mode we always drop one frame so ignore it
	if ( ( video_head_stats->dropped_frames > 0 ) & ( stat_compression == OPTIMISED ) )   
	{
		video_head_stats->dropped_frames -= 1;
	}
	video_head_stats->total_bytes_processed = (total_bytes_sent / elapsed_sec);
	video_head_stats->total_frames_count = (frames_per_second_interval / elapsed_sec);
	//Make sure that our total_frames_count is not above our input refresh rate
	if (video_head_stats->total_frames_count > context->ingress_resolution[head].refresh)
	{
		video_head_stats->total_frames_count = context->ingress_resolution[head].refresh;
	}
	video_head_stats->previous_time = current_time;
	hw_manager_calculate_average_bitrate(video_head_stats);
	hw_manager_write_video_stats_to_json(video_head_stats, stat_compression, head, peer_count);
	hw_manager_log_video_stats_to_syslog(video_head_stats, stat_compression, head, context->videoStatistics.avg_rtt_usec, peer_count);
	#ifdef ENABLE_WEB_SERVICE_REPORTING
		//MD - TODO Changed head indexing to be from 0
		statistcs_send_json_video_fact_object(&context->videoStatistics, video_head_stats, head+1);
	#endif
}

static void hw_manager_log_video_stats_to_syslog( VideoHeadStatistics * video_head_stat, const COMPRESSION_MODE cm_compression_mode, const video_head_index_e head, const uint64_t avg_rtt, const uint32_t peer_count )
{
	corrib_syslog(LOG_NOTICE,
		"H%d: CM=%s, fps=%.0f DF=%d DFT=%u Mbps=%2.2f AvgMbps=%2.2f avgFrameSize=%d FM=%lldK res=%dx%d@%dHz RTT=%llu.00us peer_count=%d",
		head,
		compression_type_name[cm_compression_mode],
		video_head_stat->frames_per_second,
		video_head_stat->dropped_frames,
		video_head_stat->total_dropped_frames,
		(video_head_stat->average_Mbytes_per_second * 8),
		(video_head_stat->moving_average_video * 8),
		video_head_stat->average_frame_size,
		get_free_ram(),
		video_head_stat->ingress_resolution->width,
		video_head_stat->ingress_resolution->height,
		video_head_stat->ingress_resolution->refresh,
		avg_rtt,
		peer_count
		);
}

void hw_manager_analogaudio_stats(hwManagerContext *hw_context)
{
	uint32_t current_time, elapsed_sec;
	FILE * fp;
	char buffer[255];
	AnalogAudioStatistics *audio = &(hw_context->analogaudioStatistics);

	current_time = sh_log_get_mstime();
	elapsed_sec = (current_time - audio->previous_time_audio) / 1000;

	/* Get audio byte sent */
	fp = fopen("/proc/aae/audio_egress/status/byte_cnt", "r");
	if (!fp) {
		corrib_syslog(LOG_ERR,"%s: Failed to open %s\n",__func__, buffer);
		return;
	}

	fgets(buffer, 20, fp);
	fclose(fp);

	audio->total_audio_bytes_sent = strtoul(buffer, NULL, 16);
	audio->audio_bytes_per_second = (float)(audio->total_audio_bytes_sent / elapsed_sec);

	if(audio->moving_average_audio == 0)
		audio->moving_average_audio = audio->audio_bytes_per_second;

	audio->moving_average_audio = hw_manager_get_rolling_average(audio->moving_average_audio, audio->audio_bytes_per_second, 1);

	if(audio->audio_bytes_per_second < audio->min_audio_bytes_per_second)
		audio->min_audio_bytes_per_second = audio->audio_bytes_per_second;

	if(audio->audio_bytes_per_second > audio->max_audio_bytes_per_second )
		audio->max_audio_bytes_per_second = audio->audio_bytes_per_second;

	statistcs_send_json_analogaudio_fact_object(&(hw_context->analogaudioStatistics));
	audio->total_audio_bytes_sent = 0;
	audio->previous_time_audio = current_time;
}

void hardware_manager_enable_all_tiles_mode(hwManagerContext *hw_context, COMPRESSION_MODE compression_mode)
{
	if( MODE_CHECK(compression_mode, LOSSLESS) )
	{
		capture_layer_set_all_tiles_mode(hw_context->capture_context, FIRST_HEAD, LOSSLESS);
	}
	if( MODE_CHECK(compression_mode, OPTIMISED) )
	{
		capture_layer_set_all_tiles_mode(hw_context->capture_context, FIRST_HEAD, OPTIMISED);
		if(hw_context->dual_head_board)
		{
			capture_layer_set_all_tiles_mode(hw_context->capture_context, SECOND_HEAD, OPTIMISED);
		}
	}

	hw_manager_set_media_suspend_state(hw_context, HEAD_ONE,false);
	hw_manager_set_media_suspend_state(hw_context, HEAD_TWO,false);
}

static void hw_manager_stop_capture_subsystem_for_compression(hwManagerContext *hw_context, int head, COMPRESSION_MODE compression_mode)
{
	corrib_syslog(LOG_DEBUG,"%s: Stopping Capture on Head %d for a compression type of %s\n", __func__, head, compression_type_name[compression_mode]);

	/* Stop capture */
	capture_layer_stop_capture(hw_context->capture_context, head, compression_mode, true);

	/* Reset Data Flow controller */
	capture_layer_data_engine_reset_head_enable(hw_context->capture_context, head, compression_mode);

	/* Wait for frames in flight and Reset */
	su_avae_enc_video_egress_reset(head, compression_mode);
}

void hardware_manager_stop_capture_subsystem(hwManagerContext *hw_context, int head, COMPRESSION_MODE cm_compression_mode)
{
	if(MODE_CHECK(cm_compression_mode, LOSSLESS)) 
	{
		hw_manager_stop_capture_subsystem_for_compression(hw_context, head, LOSSLESS);
	}
	if(MODE_CHECK(cm_compression_mode, OPTIMISED)) 
	{
		hw_manager_stop_capture_subsystem_for_compression(hw_context, head, OPTIMISED);
	}
}

static void hw_manager_init_capture_subsystem_for_compression(hwManagerContext * hw_context, int head, COMPRESSION_MODE compression_mode)
{
	corrib_syslog(LOG_DEBUG, "%s: Init %s Capture on Head %d\n", __func__, compression_type_name[compression_mode], head);
	/* Clear DF Reset bit */
	capture_layer_data_engine_reset_head_disable(hw_context->capture_context, head, compression_mode);

	/* Rate limit the capture to 0 */
	capture_layer_rate_control_disable(hw_context->capture_context, head, compression_mode);

	/* Start Capture */
	capture_layer_start_capture(hw_context->capture_context, head, compression_mode);
}

void hardware_manager_init_capture_subsystem(hwManagerContext * hw_context, int head, COMPRESSION_MODE cm_compression_mode)
{
	if(MODE_CHECK(cm_compression_mode, LOSSLESS))
	{
		hw_manager_init_capture_subsystem_for_compression(hw_context, head, LOSSLESS);
	}

	if(MODE_CHECK(cm_compression_mode, OPTIMISED))
	{
		hw_manager_init_capture_subsystem_for_compression(hw_context, head, OPTIMISED);
	}
}

void hardware_manager_start_capture_subsystem(hwManagerContext * hw_context, int head, COMPRESSION_MODE cm_compression_mode)
{
	//Bring back up the capture rate
	if(MODE_CHECK(cm_compression_mode, LOSSLESS)) {
		corrib_syslog(LOG_DEBUG, "%s: Start LOSSLESS Capture on Head %d\n", __func__, head);
		capture_layer_rate_control_enable(hw_context->capture_context, head, LOSSLESS);
	}
	if(MODE_CHECK(cm_compression_mode, OPTIMISED)) {
		corrib_syslog(LOG_DEBUG, "%s: Start OPTIMISED Capture on Head %d\n", __func__, head);
		capture_layer_rate_control_enable(hw_context->capture_context, head, OPTIMISED);
	}
}

//note: https://bboxjira.atlassian.net/browse/BUG-3585 specifies work to be done to ensure that all hm_cm_queue events 
//      are freed where appropriate. The function below (hw_manager_cleanup) may change depending on the solution for BUG-3585
void hw_manager_cleanup(hwManagerContext * context)
{
	corrib_syslog(LOG_NOTICE, "%s: deallocating %d events from hm_cm_queue", __func__, context->hm_cm_queue->count);
	eqEvent * event = NULL;
	while(context->hm_cm_queue->count > 0)
	{
		event = eq_pop(context->hm_cm_queue);
		switch(event->type)
		{
		 	case EQ_EVENT_AUDIO_COMMAND_AVAILABLE:
			event_audio_command_available_free(event);
			break;

			case EQ_EVENT_REPORT_FAULT:
			//event_report_fault_free(event);
			break;

			case EQ_EVENT_SYNC_LOSS:
			//event_sync_loss_free(event);
			break;

			case EQ_EVENT_RESOLUTION_CHANGE:
			//event_resolution_change_free(event);
			break;
			
			case EQ_EVENT_KEYBOARD_OUTPUT_REPORT:
			//event_keyboard_output_report_free(event);
			break;

			case EQ_EVENT_USB_COMMAND_AVAILABLE:
				event_usb_command_available_free((EventUsbCommandAvailable *)event);
			break;

			default:
			corrib_syslog(LOG_WARNING, "%s: unknown EQ event type (%d) found in hm_cm_queue", __func__, event->type);
			break;
		}

	}	
}
#endif