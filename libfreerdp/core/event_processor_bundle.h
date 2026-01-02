#ifndef __EVENT_PROCESSOR_VIDEO_BUNDLE_H
#define __EVENT_PROCESSOR_VIDEO_BUNDLE_H

#include <freerdp/hardware_manager.h>
#include "event_processor.h"

// ARPM: doesn't compile if this line is uncommented
// typedef struct event_processor_video_thread_bundle epVideoThreadBundle;

struct event_processor_video_thread_bundle
{
	epContext *ep_context;
	hwManagerContext * hm_context;
	UINT32 frame_number;
	int starting_tile;
	unsigned int resolution_width;
	unsigned int resolution_height;
	int tiles_changed_count;
	FRAME_TYPES frame_type;
	BOOL data_available;
	pthread_mutex_t mutex;
	int head;
	UINT32 encode_done_timestamp;
};

#endif // __EVENT_PROCESSOR_VIDEO_BUNDLE_H
