#ifndef __EVENT_PROCESSOR_H
#define __EVENT_PROCESSOR_H
#include <freerdp/hardware_manager.h>
#include "audio_packetizer.h"
#include "usb_packetizer.h"
#include <freerdp/core_event.h>

typedef enum ep_interrupt_type epInterruptType;

//forward declarations
typedef struct video_packetizer_context vpContext;
typedef struct event_processor_thread_bundles epThreadBundles;
typedef struct event_processor_video_thread_bundle epVideoThreadBundle;
//-------------------------------


enum ep_interrupt_type
{
	H1_ENCODE_COMPLETE = 1,
	H1_IRQ_RESOLUTION_CHANGE = 2,
	H1_IRQ_PARTIAL_FRAME = 3,
	H1_IRQ_SYNC_DETECT_CHANGE = 4,
	H1_IRQ_NO_OP = 5,
	H2_ENCODE_COMPLETE = 6,
	H2_IRQ_RESOLUTION_CHANGE = 7,
	H2_IRQ_PARTIAL_FRAME = 8,
	H2_IRQ_SYNC_DETECT_CHANGE = 9
};

typedef struct event_processor_context epContext;

struct event_processor_context
{

	hwManagerContext * hm_context;
	//-----------------------------------Video Processor Context ---------------------------------------
	vpContext * vp_context;
	//-----------------------------------USB Processor Context ---------------------------------------
	upContext * up_context;
	//-----------------------------------Audio Processor Context ---------------------------------------
	apContext * ap_context;
	BOOL running; //indicates status of the main worker thread
	epThreadBundles * thread_bundles;
};

typedef struct event_processor_audio_thread_bundle epAudioThreadBundle;
typedef struct event_processor_virtual_thread_bundle epVirtualThreadBundle;
typedef struct event_processor_thread_bundles epThreadBundles;

struct event_processor_thread_bundles
{
	epContext *ep_context;
	epVideoThreadBundle * head1;
	epVideoThreadBundle * head2;
	epAudioThreadBundle * audio;
	epVirtualThreadBundle ** virtuals;
};

struct event_processor_audio_thread_bundle
{

	epContext * ep_context;
	hwManagerContext * hm_context;
	BOOL data_available;
	pthread_mutex_t mutex;
};

struct event_processor_virtual_thread_bundle
{

	epContext * ep_context;
	hwManagerContext * hm_context;
	BOOL data_available;
	pthread_mutex_t mutex;
	// virtualDevice *device;
};

BOOL ep_process_krdm_event(epContext * ep_context, hwManagerContext * hm_context);
BOOL ep_process_audio_event(epContext * ep_context, hwManagerContext * hm_context);
// BOOL ep_process_virtual_event(epContext * ep_context,hwManagerContext * hm_context, virtualDevice* virtual_device);

void ep_free(epContext * ep_context, hwManagerContext * hm_context);
epContext *  ep_new();

epVideoThreadBundle *  ep_video_thread_bundle_new(epContext * ep_context,hwManagerContext * hm_context);
void ep_video_thread_bundle_free(epVideoThreadBundle * bundle);

epAudioThreadBundle *  ep_audio_thread_bundle_new(epContext * ep_context,hwManagerContext * hm_context);
void ep_audio_thread_bundle_free(epAudioThreadBundle * bundle);

// epVirtualThreadBundle ** ep_virtual_thread_bundles_new(epContext * ep_context,hwManagerContext * hm_context);
void ep_virtual_thread_bundles_free(epVirtualThreadBundle ** bundles, hwManagerContext * hm_context);

void ep_video_thread_bundle_update(epThreadBundles * bundles,EventEncodeDone * event_decode_done,BOOL state,unsigned int height, unsigned int width);

void ep_handle_resolution_change(hwManagerContext * context, int head, COMPRESSION_MODE cm_compression_mode);

#endif // __EVENT_PROCESSOR_H
