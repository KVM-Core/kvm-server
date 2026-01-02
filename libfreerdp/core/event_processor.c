/*
 * John O'Sullivan
 * Copyright: Cloudium Systems 2014
 */

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <syslog.h>
#include <sys/select.h>
#include <assert.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <freerdp/hardware_manager.h>
// #include <freerdp/bitops.h>
#include <freerdp/utils/sh_logger.h>
#include <freerdp/utils/memory.h>
// #include <freerdp/utils/hexdump.h>
#include "event_processor.h"
// #include "event_processor_bundle.h"
#include <freerdp/utils/queue.h>
// #include <freerdp/types.h>
#include <freerdp/utils/event_queue.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/utils/file.h>
#include <freerdp/core_event.h>
#include "video_packetizer.h"
#include "event_processor.h"
#include <corrib_logger.h>

#define MAX_VIDEO_EVENTS_TO_READ 6
#define MAX_AUDIO_EVENTS_TO_READ 6
#define SYNC_LOSS 0
#define MAX_NUMBER_OF_FRAMES 4
#define CUTTHROUGH_MODE 0
#define TILES_THRESHOLD_MARGIN 16 //we must leave this number of tiles to ensure that we are not taking tiles that have not finished encoding
#define FRAMES_IN_FLIGHT_WAIT_TIME 500000
#define FRAMES_IN_FLIGHT_THRESHOLD 2
#define MAX_FRAMES_IN_FLIGHT  FRAMES_IN_FLIGHT_THRESHOLD*3
static void ep_handle_sync_loss(hwManagerContext * 	hm_context,int head);
void * ep_process_events(void * arg);
void ep_process_surface_command(int head_id, epThreadBundles *thread_bundle);
void  ep_process_surface_command_h1(void * arg);
void  ep_process_surface_command_h2(void * arg);
void * ep_process_audio_command(void * arg);
void * ep_process_virtual_command(void * arg);
static pthread_t ep_create_thread( void* func, void* arg);
void ep_thread_bundles_free(epThreadBundles * bundles);
void * ep_main_loop(void * arg);
epThreadBundles *  ep_thread_bundles_new(epContext * ep_context,epVideoThreadBundle * head1,epVideoThreadBundle * head2,
										 epAudioThreadBundle * audio,epVirtualThreadBundle ** virtuals);
void ep_handle_video_frame(epContext * ep_context,hwManagerContext * hm_context,UINT32 frame_number,int starting_tile,
						   int tiles_changed_count,FRAME_TYPES frame_type,int head);
void ep_deinit_streams(epContext * ep_context, int head);

// #define RFX_SOFTWARE_PATH

/*
 * Constructor
 */
epContext * ep_new(hwManagerContext * hm_context)
{
	epContext * ep_context =  xnew(epContext,__func__);
	ep_context->hm_context = hm_context;
	ep_context->up_context = usb_packetizer_new();
	ep_context->ap_context = audio_packetizer_new();
	epVideoThreadBundle * bundle_head1 = ep_video_thread_bundle_new(ep_context,hm_context);
	epVideoThreadBundle * bundle_head2 = ep_video_thread_bundle_new(ep_context,hm_context);
	epAudioThreadBundle * bundle_audio = ep_audio_thread_bundle_new(ep_context,hm_context);
	epVirtualThreadBundle ** bundles_virtual = ep_virtual_thread_bundles_new(ep_context,hm_context);
	ep_context->thread_bundles = ep_thread_bundles_new(ep_context,bundle_head1,bundle_head2,bundle_audio,bundles_virtual);
	ep_context->running=true;
	if(ep_create_thread( ep_main_loop,ep_context) == -1)
		{
			corrib_syslog(LOG_DEBUG,"Failed to create event thread in %s, terminating\n",__func__);
			exit(0);
		}

	return ep_context;
}


/* Destructor
 *
 */
void ep_free(epContext * ep_context, hwManagerContext * hm_context)
{
	ep_context->running = false; //FIXME, is this the most appropriate place for this?
	ep_virtual_thread_bundles_free(ep_context->thread_bundles->virtuals, hm_context);
	ep_audio_thread_bundle_free(ep_context->thread_bundles->audio);
	ep_video_thread_bundle_free(ep_context->thread_bundles->head1);
	ep_video_thread_bundle_free(ep_context->thread_bundles->head2);
	ep_thread_bundles_free(ep_context->thread_bundles);
	xfree(ep_context,__func__);
}

// -------------------------------------------------------------

epVideoThreadBundle * ep_video_thread_bundle_new(epContext * ep_context,hwManagerContext * hm_context)
{
	epVideoThreadBundle * bundle =  xnew(epVideoThreadBundle,__func__);
	bundle->hm_context = hm_context;
	bundle->ep_context = ep_context;
	bundle->data_available = false;
	pthread_mutex_init(&(bundle->mutex), NULL);
	return bundle;
}
void ep_video_thread_bundle_free(epVideoThreadBundle * bundle)
{
	pthread_mutex_destroy(&(bundle->mutex));
	xfree(bundle,__func__);
}

// -------------------------------------------------------------

epAudioThreadBundle * ep_audio_thread_bundle_new(epContext * ep_context,hwManagerContext * hm_context)
{
	epAudioThreadBundle * bundle = xnew(epAudioThreadBundle,__func__);
	bundle->hm_context = hm_context;
	bundle->ep_context = ep_context;
	bundle->data_available = false;
	pthread_mutex_init(&(bundle->mutex), NULL);
	return bundle;
}

void ep_audio_thread_bundle_free(epAudioThreadBundle * bundle)
{
	pthread_mutex_destroy(&(bundle->mutex));
	xfree(bundle,__func__);
}

// -------------------------------------------------------------

// epVirtualThreadBundle * ep_virtual_thread_bundle_new(epContext * ep_context,hwManagerContext * hm_context, virtualDevice * device)
// {
// 	epVirtualThreadBundle * bundle = xnew(epVirtualThreadBundle,__func__);
// 	//corrib_syslog(LOG_INFO ,"ep_virtual_thread_bundle_new start\n");
// 	bundle->hm_context = hm_context;
// 	bundle->ep_context = ep_context;
// 	bundle->data_available = false;
// 	pthread_mutex_init(&(bundle->mutex), NULL);
// 	bundle->device = device;
// 	//corrib_syslog(LOG_INFO ,"ep_virtual_thread_bundle_new end\n");

// 	return bundle;
// }

epVirtualThreadBundle ** ep_virtual_thread_bundles_new(epContext * ep_context,hwManagerContext * hm_context)
{
	epVirtualThreadBundle ** bundles = NULL; int index;
	// virtualInterface * virtual_interface = &hm_context->virtual_interface;

	// if(virtual_interface->size)
	// {
	// 	bundles = (epVirtualThreadBundle**) xzalloc(sizeof(epVirtualThreadBundle*) * virtual_interface->size,__func__);
	// 	for(index = 0; index < virtual_interface->size; index++)
	// 	{
	// 		bundles[index] = ep_virtual_thread_bundle_new(ep_context, hm_context, &virtual_interface->devices[index]);
	// 	}
	// }

	return bundles;
}


void ep_virtual_thread_bundle_free(epVirtualThreadBundle * bundle)
{
	pthread_mutex_destroy(&(bundle->mutex));
	xfree(bundle,__func__);
}

void ep_virtual_thread_bundles_free(epVirtualThreadBundle ** bundles, hwManagerContext * hm_context)
{
	int index;
	int bundles_count;

	// bundles_count = hm_context->virtual_interface.size;

	// for(index = 0; index < bundles_count; index++)
	// {
	// 	ep_virtual_thread_bundle_free(*bundles++);
	// }

	xfree(bundles,__func__);
}


// epVirtualThreadBundle * ep_virtual_thread_get_bundle(epVirtualThreadBundle ** bundles, hwManagerContext * hm_context, virtualDevice *device)
// {
// 	epVirtualThreadBundle * bundle = NULL;

// 	int index, bundles_count = hm_context->virtual_interface.size;

// 	for(index = 0; index < bundles_count; index++)
// 	{
// 		//corrib_syslog(LOG_DEBUG,"%s: index = %d\n",  __func__, index);
// 		if(bundles[index]->device == device)
// 		{
// 			bundle = bundles[index];
// 			//corrib_syslog(LOG_DEBUG,"%s: bundle = %x\n",  __func__,bundle);
// 			break;
// 		}
// 	}

// 	return bundle;
// }

// -------------------------------------------------------------

epThreadBundles * ep_thread_bundles_new(epContext * ep_context, epVideoThreadBundle * head1, epVideoThreadBundle * head2,
										epAudioThreadBundle * audio, epVirtualThreadBundle ** virtuals)
{
	epThreadBundles * bundles = xnew(epThreadBundles,__func__);
	bundles->ep_context = ep_context;
	bundles->head1 = head1;
	bundles->head2 = head2;
	bundles->audio = audio;
	bundles->virtuals = virtuals;
	return bundles;
}


void ep_thread_bundles_free(epThreadBundles * bundles)
{
	xfree(bundles,__func__);
}


void ep_audio_thread_bundle_update(epAudioThreadBundle * audio_bundle)
{
	pthread_mutex_lock(&(audio_bundle->mutex));
	audio_bundle->data_available = true;
	pthread_mutex_unlock(&(audio_bundle->mutex));
}

void ep_virtual_thread_bundle_update(epVirtualThreadBundle * virtual_bundle)
{
	pthread_mutex_lock(&(virtual_bundle->mutex));
	virtual_bundle->data_available = true;
	pthread_mutex_unlock(&(virtual_bundle->mutex));
}

void ep_video_thread_bundle_update(epThreadBundles * bundles, EventEncodeDone * event_decode_done, BOOL state, unsigned int height, unsigned int width)
{
	epVideoThreadBundle *bundle = (event_decode_done->head_id == 1) ? bundles->head1 : bundles->head2;
	pthread_mutex_lock(&(bundle->mutex));
	bundle->head = event_decode_done->head_id;
	bundle->frame_number = event_decode_done->frame_number;
	bundle->frame_type = event_decode_done->frame_type;
	bundle->starting_tile = event_decode_done->starting_tile;
	bundle->tiles_changed_count = event_decode_done->number_of_tiles;
	bundle->resolution_height = height;
	bundle->resolution_width = width;
	bundle->data_available = true;
	bundle->encode_done_timestamp = sh_log_get_mstime();
	pthread_mutex_unlock(&(bundle->mutex));
}

/*
 * This needs to be capable of dynamically adding fds from multiple sources, for example peers who will come and go
 */
static BOOL ep_get_fds(hwManagerContext  * hm_context, void** rfds, int* rcount)
{
	//int i;
	//get the queue FD for the hardware manager as we must process his events
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"Event Processor has exceeded maximum file descriptors (%d) in %s\n",MAX_FDS, __func__);
		return false;
	}
	int fd_hm_ep = eq_get_queue_fd(hm_context->hm_ep_queue);
	if (fd_hm_ep < 1)
		return false;
	else
	{
		rfds[*rcount] = (void*)(long)(fd_hm_ep);
		(*rcount)++;
	}
	return true;
}

static void ep_set_exit_state(hwManagerContext * context,exitState state,const char * message)
{
	context->hm_exit_state = state;
	strncpy(context->exit_info,message,255);

}

void ep_process_interval_tasks(hwManagerContext * context)
{

}

void ep_get_timeout_interval(hwManagerContext * context, struct timeval* tv)
{
	if(context != NULL && tv != NULL)
	{
	    tv->tv_sec = HW_DEFAULT_INTERVAL_PERIOD;
	    tv->tv_usec = 0;
	}
}

void * ep_main_loop(void * arg)
{
	epContext * ep_context = (epContext *)arg;
	epThreadBundles * ep_bundles = ep_context->thread_bundles;
	hwManagerContext * 	context = ep_bundles->head1->hm_context;
	BOOL running = true;
	int num_set;
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[MAX_FDS];
	fd_set rfds_set;
	memset(rfds, 0, sizeof(rfds));

	struct timeval tv = {.tv_sec = HW_DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //
	context->main_thread_state = RUNNING;
	assert(context->cm_hm_queue);
	time_t last_processed = time(NULL);
	while(running)
	{
		rcount = 0;

		ep_get_timeout_interval(context, &tv);
		if (ep_get_fds(context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"Failed to get Event Processor file descriptors in %s\n",__func__);
			running = false;
			ep_set_exit_state(context,ERROR,"Failed to get FDs\n");
			context->main_thread_state = STOPPED; //FIXME, do we want to stop the main thread or this one
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
			context->main_thread_state = STOPPED; //FIXME, do we want to stop the main thread or this one
			corrib_syslog(LOG_ERR,"max fds are zero in %s\n",__func__);
			ep_set_exit_state(context,ERROR,"max_fds were 0\n");
			break;
		}


		//corrib_syslog(LOG_DEBUG,"HM_BS\n");
		num_set = select(max_fds + 1, &rfds_set, NULL, NULL, &tv);
		//corrib_syslog(LOG_DEBUG,"HM_AS\n");
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
			BOOL known_event = false;
			int hm_ep_fd = eq_get_queue_fd(context->hm_ep_queue);

			if (FD_ISSET(hm_ep_fd, &rfds_set))
			{
				known_event = true;
				//read the queue
				eqEvent* event = eq_pop(context->hm_ep_queue);
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
						ep_set_exit_state(context,NORMAL,"Normal exit, no errors\n");
						context->main_thread_state = STOPPED; //FIXME, do we want to stop the main thread or this one
						break;
					case EQ_EVENT_VIRTUAL_DONE:

						if(context->debug_enabled) {
							corrib_syslog(LOG_DEBUG,"%s: got an EQ_EVENT_VIRTUAL_DONE\n",  __func__);
						}

						EventVirtualDone * event_virtual_done = (EventVirtualDone * )event;

						// Uncomment
						// epVirtualThreadBundle * virtual_bundle = ep_virtual_thread_get_bundle(ep_context->thread_bundles->virtuals,
						// 																	  context, event_virtual_done->device);
						// if(virtual_bundle)
						// {
						// 	ep_virtual_thread_bundle_update(virtual_bundle);
						//     ep_process_virtual_command(virtual_bundle);
						// }
						break;

					case EQ_EVENT_AUDIO_DONE:
                    {
						EventAudioDone * event_audio_done = (EventAudioDone * )event;
						ep_audio_thread_bundle_update(ep_context->thread_bundles->audio);
						ep_process_audio_command(ep_context->thread_bundles->audio);
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
			if(((tv.tv_sec == 0) && (tv.tv_usec == 0)) || (last_processed + HW_DEFAULT_INTERVAL_PERIOD < time(NULL)) ) // this will timeout if we have no other events
			{
				known_event = true;
				ep_process_interval_tasks(context);
				last_processed = time(NULL);
			}
			if(known_event == false)
				corrib_syslog(LOG_ERR,"%s: got an unknown event from select.\n",  __func__);
			}


	} //end of while loop
	//ep_free(ep_context, context); //FIXME, Double Free here
	pthread_exit(NULL);

	return NULL;
	return 0;
}

static void ep_handle_sync_loss(hwManagerContext * context, int head)
{
	corrib_syslog(LOG_DEBUG , "SYNC LOSS interrupt processed in %s\n",__func__);
	EventSyncLoss * sync_loss_event = NULL;

	sync_loss_event = event_sync_loss_new(head);

	hw_manager_clear_video_head_structs(context, head-1);
	
	if(context->performance_analysis)
		sync_loss_event->send_time = sh_log_get_mstime();

	/* RESET DF, RESET AVAE and Stop Capture for specific head */
	hardware_manager_stop_capture_subsystem(context, head-1, context->configured_compression);

	/* Send event to the connection manager EQ_EVENT_SYNC_LOSS */
	eq_push(context->hm_cm_queue,(eqEvent *)sync_loss_event);
}

void ep_handle_resolution_change(hwManagerContext * context, int head, COMPRESSION_MODE cm_compression_mode)
{
	videoData_t connectionResData;
	/* Resolution events are handled and created by RPU0 at this
	 * point we can be confident that no error handling is needed here
	 */
	if((hw_manager_initialise_fpga(context, head-1, PASS1) == false))
	{
		corrib_syslog(LOG_ERR, "%s(): hw_manager_initialise_fpga failed\n",__func__);
	}

	hardware_manager_init_capture_subsystem(context, head-1, cm_compression_mode);

	corrib_syslog(LOG_NOTICE, "%s:Send Res Change Event for Head %d\n", __func__, head);

	connectionResData.ingress_resolution = context->ingress_resolution[head-1];
	connectionResData.optimised_egress_res = context->optimised_egress_res[head-1];
	if( FIRST_HEAD == (head-1) )
	{
		connectionResData.lossless_egress_res = context->lossless_egress_res;
	}
	else
	{
		connectionResData.lossless_egress_res = (videoHead_t){0, 0 ,0};
	}
	connectionResData.sync_loss = (!connectionResData.ingress_resolution.height && !connectionResData.ingress_resolution.width);
	EventResolutionChange * resolution_change_event = event_resolution_change_new(head, connectionResData);

	if(context->performance_analysis)
		resolution_change_event->send_time = sh_log_get_mstime();

	eq_push(context->hm_cm_queue,(eqEvent *)resolution_change_event); //send event to the connection manager
}

static void ep_frame_type_to_string(char * buffer, FRAME_TYPES frame_type)
{
	if(frame_type == FULL_FRAME)
		snprintf(buffer,20,"full frame");
	else if(frame_type == PARTIAL)
		snprintf(buffer,20,"partial frame");
	else if(frame_type == PARTIAL_START)
		snprintf(buffer,20,"partial start");
	else
		snprintf(buffer,20,"full frame");
}


static pthread_t ep_create_thread( void* func, void* arg)
{
	pthread_t thread;
	if(pthread_create(&thread, 0, func, arg) != 0)
	{
		corrib_syslog(LOG_ERR,"%s: Failed to create thread for event processor\n",__func__);
		return -1;
	}
	else
	{

		return thread;
	}
	return 0;
}


/*
 * process audio
 * Package up the audio here into a form suitable for transmission
 */
void * ep_process_audio_command(void * arg)
{
	epAudioThreadBundle * ep_bundle = (epAudioThreadBundle *)arg;
	AUDIO_DATA_COMMAND * cmd = NULL; // represents audio command, the connection manager decides who sends the audio
	//corrib_syslog(LOG_DEBUG,"************ HARDWARE AUDIO EVENT PROCESS%s\n",__func__);
	if(ep_bundle->hm_context->suspend_audio == false) //FIXME, we need a way of suspending audio
	{
		cmd = ap_create_audio_command(ep_bundle->ep_context->ap_context,ep_bundle->hm_context);
		if(cmd != NULL)
		{
			EventAudioCommandAvailable * audio_command_available_event = event_audio_command_available_new(cmd); //create an end event to stop the hardware manager
			eq_push(ep_bundle->hm_context->hm_cm_queue, (eqEvent *)audio_command_available_event);
		}
	}
	else
	{
		//do whatever you may need to do here to dispose of teh audio, maybe nothing is required, this is just a place holder
	}
	return NULL;
}

/*
 * process virtual
 * Package up the virtual here into a form suitable for transmission
 */
void * ep_process_virtual_command(void * arg)
{
	// epVirtualThreadBundle * ep_bundle = (epVirtualThreadBundle *)arg;
	// upCommandResult cmdResult;
	// //sequenceData sequence_data;
	// //USB_COMMAND * cmd;
	// if(ep_bundle->hm_context->suspend_virtual == false)
	// {
	// 	// Need some way of grabbing sequenceType at this point
	// 	//sequence_data = up_create_usb_sequence_data(ep_bundle->ep_context->up_context, ep_bundle->hm_context, ep_bundle->fd);
	// 	//switch()
	// 	cmdResult = up_create_usb_command(ep_bundle->ep_context->up_context, ep_bundle->hm_context, ep_bundle->device->fd);
	// 	if(cmdResult.sequence_data != NULL)
	// 	{
	// 		switch(cmdResult.sequence_data->type)
	// 		{
	// 			case USBSQT_RELEASE_RESOURCE:
	// 			// Decouples the virtual FD from the Device Id
	// 			//corrib_syslog(LOG_DEBUG,"* USBSQT_RELEASE_RESOURCE device_id %d :%s\n",ep_bundle->device->device_id,__func__);
	// 			if(cmdResult.cmd != NULL || (ep_bundle->device->device_id = 0))
	// 			{
	// 				EventUsbCommandAvailable * usb_command_available_event = event_usb_command_available_new(cmdResult.sequence_data, ep_bundle->device->device_id, cmdResult.cmd); //create an end event to stop the hardware manager
	// 				if(ep_bundle->hm_context->performance_analysis)
	// 				{
	// 					usb_command_available_event->send_time = sh_log_get_mstime();
	// 				}
	// 				ep_bundle->device->device_id = 0;
	// 				if (!usb_command_available_event)
	// 				{
	// 					corrib_syslog(LOG_ERR, "%s(): USBSQT_RELEASE_RESOURCE command memory allocation failed. Possible memory leak condition.\n",  __func__);
	// 				}
	// 				else
	// 				{
	// 					eq_push(ep_bundle->hm_context->hm_cm_queue, (eqEvent *)usb_command_available_event);
	// 				}
	// 			}
	// 			break;
	// 			default:
	// 			if(cmdResult.cmd != NULL)
	// 			{
	// 				EventUsbCommandAvailable * usb_command_available_event = event_usb_command_available_new(cmdResult.sequence_data, ep_bundle->device->device_id, cmdResult.cmd); //create an end event to stop the hardware manager
	// 				if(ep_bundle->hm_context->performance_analysis)
	// 				{
	// 					usb_command_available_event->send_time = sh_log_get_mstime();
	// 				}
	// 				if (!usb_command_available_event)
	// 				{
	// 					corrib_syslog(LOG_ERR, "%s(): Type [%d] command memory allocation failed. Possible memory leak condition.\n",  __func__, cmdResult.sequence_data->type);
	// 				}
	// 				else
	// 				{
	// 					eq_push(ep_bundle->hm_context->hm_cm_queue, (eqEvent *)usb_command_available_event);
	// 				}
	// 			}
	// 			break;
	// 		}
	// 	}

	// }

	return NULL;
}

void ep_handle_audio_frame(epContext * ep_context,hwManagerContext * hm_context)
{
	EventAudioDone * audio_done_event = event_audio_done_new();
	if(hm_context->performance_analysis)
		audio_done_event->send_time = sh_log_get_mstime();
	eq_push(hm_context->hm_ep_queue,(eqEvent *)audio_done_event); //send event to the event processor
}


void ep_handle_video_frame(epContext * ep_context,hwManagerContext * hm_context,UINT32 frame_number,int starting_tile,int tiles_changed_count,FRAME_TYPES frame_type,int head)
{

//	if(head == 2)
//		corrib_syslog(LOG_DEBUG,"********* got a video event for head %d to process:%s\n",head,__func__);
	//ep_video_thread_bundle_update(ep_context->thread_bundles,frame_number,starting_tile,tiles_changed_count,frame_type,head,true);
	EventEncodeDone * decode_done_event = event_encode_done_new(head,frame_type,frame_number,starting_tile,tiles_changed_count);
	if(hm_context->performance_analysis)
		decode_done_event->send_time = sh_log_get_mstime();
	eq_push(hm_context->hm_ep_queue,(eqEvent *)decode_done_event); //send event to the event processor

}

// void ep_handle_virtual_frame(epContext * ep_context,hwManagerContext * hm_context, virtualDevice* virtual_device)
// {
// 	EventVirtualDone * virtual_done_event = event_virtual_done_new(virtual_device);
// 	if(hm_context->performance_analysis)
// 		virtual_done_event->send_time = sh_log_get_mstime();
// 	eq_push(hm_context->hm_ep_queue,(eqEvent *)virtual_done_event); //send event to the event processor
// }


//Event Processing for Audio
//----------------------------------------------
BOOL ep_process_audio_event(epContext* ep_context, hwManagerContext* hm_context)
{
	BOOL status = true;
	//corrib_syslog(LOG_DEBUG,"************ HARDWARE AUDIO EVENT %s\n",__func__);
	ep_handle_audio_frame(ep_context,hm_context);

	return status;
}

// //Event Processing for Virtual
// //----------------------------------------------
// BOOL ep_process_virtual_event(epContext * ep_context,hwManagerContext * hm_context, virtualDevice* virtual_device)
// {
// 	BOOL status = true;

// 	ep_handle_virtual_frame(ep_context,hm_context,virtual_device);
// 	return status;
// }

//Event Processing for Video
//----------------------------------------------
BOOL ep_process_krdm_event(epContext * ep_context, hwManagerContext * hm_context)
{
	BOOL status = true;
	epInterruptType kernel_irq_type;
	int kernel_event_value;
	char copy_buffer[3];
	int i;
	UINT32 frame_number;
	int n;
	char result[MAX_VIDEO_EVENTS_TO_READ]; //need to size this appropriately
	//corrib_syslog(LOG_DEBUG,"EP_BR\n");
	int bytesRead = read(hm_context->krdm_fd, result, MAX_VIDEO_EVENTS_TO_READ);
	//corrib_syslog(LOG_DEBUG,"EP_AR\n");
	if(hm_context->debug_enabled)
		corrib_syslog(LOG_INFO,"_________________%s: ep debug enabled_____________ \n", __func__);
	if (bytesRead < 0) //invalid byte count
	{
		if(!((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
			 (errno == EINPROGRESS)/* || (errno == EINTR)*/))
		{
			corrib_syslog(LOG_ERR,"%d %s: read failed on error in: %s.\n", errno, strerror(errno), __func__);
			status = false;
			return status;
		}

	}
	else //valid byte count
	{
		if (bytesRead == 0 || (bytesRead%2 != 0)) //not sure if this is an actual error, could be unblocked due to signal, need to confirm
		{
			corrib_syslog(LOG_ERR,"Failed to read data from Kernel Pipe, received %d bytes in %s\n",bytesRead,__func__);
			status = false;
			return status;
		}
		for(n=0; n < bytesRead; n+=2) //we may have up to 6 events here so after a resolution change we want to break from this and clear the kernel pipe
		{
			//MD - TODO Are we sure only ASCII can be returned here???
			kernel_irq_type = result[n] - 0x30;
			kernel_event_value = result[n+1] - 0x30;
			frame_number=kernel_event_value;
			//corrib_syslog(LOG_DEBUG,"EP_%d %d\n",kernel_irq_type,kernel_event_value);
			if(kernel_irq_type == H1_IRQ_RESOLUTION_CHANGE)
			{
				corrib_syslog(LOG_NOTICE ,"H1_IRQ_RESOLUTION_CHANGE\n");
				event_encode_done_video_purge( hm_context->hm_cm_queue, 1);//remove any stale events associated with video
				eq_purge(EQ_EVENT_MOUSE, hm_context->cm_hm_queue); //remove any stale events associated with the mouse

				ep_handle_resolution_change(hm_context, 1, hm_context->configured_compression);
			}
			else if(kernel_irq_type == H2_IRQ_RESOLUTION_CHANGE)
			{
				corrib_syslog(LOG_NOTICE ,"H2_IRQ_RESOLUTION_CHANGE\n");
				event_encode_done_video_purge( hm_context->hm_cm_queue, 2);//remove any stale events associated with video
				eq_purge(EQ_EVENT_MOUSE, hm_context->cm_hm_queue); //remove any stale events associated with the mouse

				ep_handle_resolution_change(hm_context, 2, hm_context->configured_compression);
			}
			else if(kernel_irq_type == H1_IRQ_SYNC_DETECT_CHANGE)
			{
				corrib_syslog(LOG_NOTICE ,"H1_IRQ_SYNC_DETECT_CHANGE\n");
				event_encode_done_video_purge( hm_context->hm_cm_queue, 1);//remove any stale events associated with video
				ep_handle_sync_loss(hm_context,1);
			}
			else if(kernel_irq_type == H2_IRQ_SYNC_DETECT_CHANGE)
			{
				corrib_syslog(LOG_NOTICE ,"H2_IRQ_SYNC_DETECT_CHANGE\n");
				event_encode_done_video_purge( hm_context->hm_cm_queue, 2);//remove any stale events associated with video
				//eq_purge(EQ_EVENT_DECODE_DONE, hm_context->hm_cm_queue); //remove any stale events associated with video
				ep_handle_sync_loss(hm_context,2); //FIXME, disabling for debug
			}
			else if(kernel_irq_type == H1_IRQ_NO_OP)
			{

				//we do nothing here we have purged the kernel pipe of video events
				corrib_syslog(LOG_INFO,"Got NoOp event\n");
			}
			else
			{
				// corrib_syslog(LOG_ERR, "%s:Got interrupt of type (%d), which I cannot understand, second byte was %d\n",__func__,kernel_irq_type,kernel_event_value);
			}
			//corrib_syslog(LOG_DEBUG,"**** EP_%d %d\n",kernel_irq_type,kernel_event_value);
		} // for(n=0; n < bytesRead; n+=2)
	} //valid byte count end

	return status;
}
