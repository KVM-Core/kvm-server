#ifndef __VIDEO_PACKETIZER_H
#define __VIDEO_PACKETIZER_H

#include <freerdp/codec/rfx.h>
#include <freerdp/update.h>
#include <freerdp/listener.h>
// #include <freerdp/utils/stream.h>
#include <freerdp/utils/stopwatch.h>
#include <time.h>
#include "event_processor.h"
#include "event_processor_bundle.h"


typedef struct video_packetizer_context vpContext;


struct video_packetizer_context
{
	//these are used for statistics
	UINT32 total_bytes_sent;
	UINT32 total_tiles_changed;
	UINT32 total_tiles_per_frame;
	UINT32 total_frames_count;
	UINT32 total_frames_count_previous; //may include partial frames
	UINT32 total_rfx_frames_count; //may include partial frames
	UINT32 total_rfx_frames_count_previous; // only counts full rfx frame which may be composed of a number of partial frames
	UINT32 total_bytes_sent_previous; // only counts full rfx frame which may be composed of a number of partial frames
	UINT32 total_tiles_changed_previous;
	float bytes_per_second;
	float frames_per_second;
	float rfx_frames_per_second;
	float average_tiles_per_frame;
	UINT32 previous_time;
	int dropped_frames;
	UINT32 bad_frames;

	float average_tile_size;
	UINT32 total_tiles_sent;
	UINT32 total_tile_size;

	BOOL log_data;

	//-----------------------------------RFX Context ---------------------------------------
	RFX_CONTEXT* rfx_context;
};


vpContext *  video_packetizer_new();
void video_packetizer_free(vpContext * vp_context);
SURFACE_BITS_COMMAND * vp_create_surface_command(vpContext * vp_context, epVideoThreadBundle * ep_bundle,UINT32 * size);
void vp_dump_surface_bits_header(SURFACE_BITS_COMMAND* cmd, const char * comment);
void vp_init(vpContext * vp_context, hwManagerContext * hm_context);
void vp_deinit_streams(vpContext * vp_context, int head);
#endif //__VIDEO_PACKETIZER_H
