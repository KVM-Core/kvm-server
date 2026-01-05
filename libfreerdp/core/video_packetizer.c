/*
 * John O'Sullivan
 * Copyright: Cloudium Systems 2014
 */

#include "video_packetizer.h"
#include <freerdp/hardware_manager.h>
#include <freerdp/locale/keyboard.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/video_frame.h>
#include <freerdp/codec/rfx_constants.h>
#include <freerdp/utils/file.h>
#include <freerdp/utils/test.h>
#include <freerdp/utils/sleep.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/thread.h>
#include <freerdp/utils/sh_logger.h>
#include <freerdp/update.h>
#include "event_processor.h"
#include <corrib_logger.h>

/*
 * takes tile data and transforms it into a surface command packet
 * use hw_manager context and rfx_context to do this
 *
 */




/*
 * Constructor
 */
vpContext *  video_packetizer_new()
{
	vpContext * vp_context =  xnew(vpContext,__func__);
	vp_context->rfx_context = rfx_context_new();
	//rfx_context_init_video_frames(vp_context->rfx_context,1); //head 1
	//rfx_context_init_video_frames(vp_context->rfx_context,2); //head 2
	return vp_context;
}




/* Destructor
 *
 */
void video_packetizer_free(vpContext * vp_context)
{
	rfx_context_free(vp_context->rfx_context);
	xfree(vp_context,__func__);
}

/*
static void video_packetizer_calculate_average_bitrate(vpContext * vp_context, uint32 bytes_sent,uint32 tiles_changed,FRAME_TYPES frame_type)
{

	uint32 current_time;
	uint32 time_difference;
	current_time= get_mstime();
	vp_context->total_bytes_sent += bytes_sent;
	vp_context->total_frames_count++;
	if((frame_type == 	FULL_FRAME) || 	(frame_type ==	PARTIAL_END))
	{
		vp_context->total_rfx_frames_count++;
	}

	vp_context->total_tiles_changed += tiles_changed;

	time_difference = current_time - vp_context->previous_time;
	if(time_difference > 60000) // more than a minute has elapsed
	{
		vp_context->previous_time = current_time;
		uint32 frame_difference = vp_context->total_frames_count - vp_context->total_frames_count_previous;
		uint32 rfx_frame_difference = vp_context->total_rfx_frames_count - vp_context->total_rfx_frames_count_previous;
		uint32 bytes_difference = vp_context->total_bytes_sent - vp_context->total_bytes_sent_previous;
		uint32 tiles_difference = vp_context->total_tiles_changed - vp_context->total_tiles_changed_previous;
		vp_context->bytes_per_second = (float)((float)bytes_difference/60);
		vp_context->frames_per_second = (float)((float)frame_difference/60);
		vp_context->rfx_frames_per_second = (float)((float)rfx_frame_difference/60);
		vp_context->average_tiles_per_frame = tiles_difference/frame_difference;
		if(!vp_context->log_data) //only do this if logging is off as logging uses dropped frame per frame stats
		{
			vp_context->dropped_frames = fpga_get_dropped_frame_count(vp_context->rfx_context)/60;
			fpga_clear_dropped_frame_count(vp_context->rfx_context);
		}
		if(vp_context->bytes_per_second > vp_context->server_settings->mouse_throttle_threshold)
		{
			vp_context->throttle_mouse = true;
		}
		else
			vp_context->throttle_mouse = false;
		printf("rfxfps=%2.2f fps=%2.2f Bps=%2.2f avgTPF=%2.2f avgTSz=%2.2f DF per sec= %d, bad=%d\n",
				vp_context->rfx_frames_per_second,
				vp_context->frames_per_second,
				vp_context->bytes_per_second,
				vp_context->average_tiles_per_frame,
				vp_context->average_tile_size,
				vp_context->dropped_frames,
				vp_context->bad_frames);
		if(0)
		{ //TEMPORARY to dump stats for BB analysis
			FILE * fileptr  = fopen("/usr/local/statistics.txt","a+");
			fprintf(fileptr,"fps=%2.2f Bps=%2.2f avgTPF=%2.2f avgTSz=%2.2f\n",
							vp_context->frames_per_second,
							vp_context->bytes_per_second,
							vp_context->average_tiles_per_frame,
							vp_context->average_tile_size);
			fclose(fileptr);
		}
		vp_context->previous_time = current_time;
		vp_context->total_bytes_sent_previous = vp_context->total_bytes_sent;
		vp_context->total_frames_count_previous = vp_context->total_frames_count;
		vp_context->total_rfx_frames_count_previous = vp_context->total_rfx_frames_count;
		vp_context->total_tiles_changed_previous = vp_context->total_tiles_changed;

		vp_context->total_tiles_sent = 0;
		vp_context->total_tile_size = 0;
		vp_context->dropped_frames = 0;

	}


}

*/

/*
 * Copies the quantization data from the hm_context into the rfx_context
 */
void vp_init(vpContext * vp_context, hwManagerContext * hm_context)
{

	RFX_CONTEXT * rfx_context = vp_context->rfx_context;
	vp_context->rfx_context->mode = RLGR1; // was originally set to RLGR3
	rfx_context_set_pixel_format(rfx_context, RDP_PIXEL_FORMAT_R8G8B8);
	rfx_context->quant_idx_cb = hm_context->quant_idx_cb;
	rfx_context->quant_idx_cr = hm_context->quant_idx_cr;
	rfx_context->quant_idx_y = hm_context->quant_idx_y;
	rfx_context->quants = hm_context->quants;
	rfx_context->num_quants = hm_context->num_quants;
}

void vp_deinit_streams(vpContext * vp_context, int head)
{
	rfx_context_deinit_streams(vp_context->rfx_context,head);
}

int vp_get_active_buffer_index(vpContext * vp_context,int head)
{
	if(head)
		return vp_context->rfx_context->active_frame_buffer_h1;
	else
		return vp_context->rfx_context->active_frame_buffer_h2;
}

int vp_get_active_buffer_status(vpContext * vp_context,int head)
{
	if(head)
		return vp_context->rfx_context->video_frame_h1->in_use;
	else
		return vp_context->rfx_context->video_frame_h2->in_use;
}

void vp_set_active_buffer_status(vpContext * vp_context,int head,BOOL status)
{
	if(head)
		vp_context->rfx_context->video_frame_h1->in_use = status;
	else
		vp_context->rfx_context->video_frame_h2->in_use = status;
}
void vp_increment_active_buffer(vpContext * vp_context,int head)
{
	RFX_CONTEXT * rfx_context = vp_context->rfx_context;
	if(head)
	{
		rfx_context->active_frame_buffer_h1 = rfx_context->active_frame_buffer_h1 + 1;
		//if(rfx_context->active_frame_buffer_h1 > 1)
		//	corrib_syslog(LOG_DEBUG,"Using %d buffers on head %d\n",rfx_context->active_frame_buffer_h1,1);
		if(rfx_context->active_frame_buffer_h1 > (MAX_FRAME_BUFFERS - 1))
			rfx_context->active_frame_buffer_h1 = 0;
		//rfx_context->video_frame_buffers_h1[rfx_context->active_frame_buffer_h1]->in_use = true;
	}
	else
	{
		rfx_context->active_frame_buffer_h2 = rfx_context->active_frame_buffer_h2 + 1;
		//if(rfx_context->active_frame_buffer_h1 > 1)
		//	corrib_syslog(LOG_DEBUG,"Using %d buffers on head %d\n",rfx_context->active_frame_buffer_h1,1);
		if(rfx_context->active_frame_buffer_h2 > (MAX_FRAME_BUFFERS - 1))
			rfx_context->active_frame_buffer_h2 = 0;
		//rfx_context->video_frame_buffers_h1[rfx_context->active_frame_buffer_h1]->in_use = true;
	}

}

SURFACE_BITS_COMMAND * vp_create_surface_command(vpContext * vp_context,epVideoThreadBundle * ep_bundle,uint32 * size)
{
	hwManagerContext * hm_context = ep_bundle->hm_context;
	int start_tile = ep_bundle->starting_tile;
	int end_tile = ep_bundle->tiles_changed_count;
	int head = ep_bundle->head;
	int frame_number = ep_bundle->frame_number;
	STREAM * s_main;
	STREAM * s_headers;
	RFX_RECT rect;

	SURFACE_BITS_COMMAND * cmd = surface_bits_command_new();

	int num_tiles_changed = 0;
	uint16 rfx_width;
	uint16 rfx_height;
	RFX_CONTEXT * rfx_context = vp_context->rfx_context;

	if(head == 1)
	{

		s_main = cmd->video_frame->s_main;
		s_headers = cmd->video_frame->s_headers;
		//pthread_mutex_lock(&(hm_context->mutex));
		//hw_manager_increment_frames_in_flight(hm_context,__func__, head);
		//corrib_syslog(LOG_DEBUG,">>>> Incrementing frames_in_flight_h1 %u\n",hm_context->frames_in_flight_h1);
		//pthread_mutex_unlock(&(hm_context->mutex));

		rfx_context->video_frame_h1 = cmd->video_frame;
		//hw_manager_get_resolution(hm_context,&rfx_context->width_h1,&rfx_context->height_h1,head);
		rfx_context->width_h1 = ep_bundle->resolution_width;
		rfx_context->height_h1 = ep_bundle->resolution_height;
		rfx_width = rfx_context->width_h1;
		rfx_height = rfx_context->height_h1;
	}
	else
	{


		//rfx_context->video_frame_h2->in_use = true;
		//s_main = rfx_context->video_frame_h2->s_main;
		//s_headers = rfx_context->video_frame_h2->s_headers;
		s_main = cmd->video_frame->s_main;
		s_headers = cmd->video_frame->s_headers;
		//hw_manager_increment_frames_in_flight(hm_context,__func__, head);

		rfx_context->video_frame_h2 = cmd->video_frame;
		//hw_manager_get_resolution(hm_context,&rfx_context->width_h2,&rfx_context->height_h2,head);
		rfx_context->width_h2 = ep_bundle->resolution_width;
		rfx_context->height_h2 = ep_bundle->resolution_height;
		rfx_width = rfx_context->width_h2;
		rfx_height = rfx_context->height_h2;
	}

	if (rfx_width * rfx_height <= 0)
	{
		corrib_syslog(LOG_ERR,"%s:rfx context has invalid resolution %dX%d\n",__func__,rfx_width,rfx_height);
		return NULL;
	}
	cmd->destRight = rfx_width;  //rfx_context->width is set fro resolution detection, xfp->image_x is always zero
	cmd->destBottom = rfx_height;
	cmd->bpp = 32;
	cmd->codecID = CODEC_ID_REMOTEFX_CLOUDIUM; //client->settings->rfx_codec_id; //this is where we set the codec type RFX or RFX_CLOUDIUM, normall retrieved from client but we may want to set it here to cloudium
	cmd->width = rfx_width;
	cmd->height = rfx_height;
	//populate the rectangle
	rect.x = 0;
	rect.y = 0;
	rect.width = rfx_width;
	rect.height = rfx_height;

	assert(s_headers);
	stream_set_pos(s_headers,0); //reset the headers stream
	stream_set_pos(s_main,0); //reset the main stream
	//MD - Removed this call 01/10/2020
	//num_tiles_changed = enc_fpga_read_n_tile_headers(hm_context,frame_number,s_headers,start_tile, end_tile,head);

	stream_set_pos(s_headers,0); //reset the headers stream
	//freerdp_hexdump(s_headers->p,64);
	//enc_fpga_dump_tile_headers(rfx_context,s_headers, num_tiles_changed,head);
	//enc_fpga_clear_changed_tile_count(hm_context,frame_number,head);
	//corrib_syslog(LOG_DEBUG,"Read %d tiles from %d to %d for frame %d\n",numTilesChanged,start_tile,end_tile,xfp->current_frame_number);
// 	if(num_tiles_changed > 0)
// 	{
// 		if((frame_number < NUMBER_OF_FRAMES))
// 		{
// #if 1

// 			if(rfx_compose_message_rdvh_from_list(rfx_context,hm_context, &rect, 1, num_tiles_changed,frame_number,head) == -1)
// 			{
// 				corrib_syslog(LOG_ERR,"Error in %s: Composing RFX message\n",__func__);
// 				return NULL;
// 			}
// #endif

// 		}
// 		else
// 		{
// 			corrib_syslog(LOG_ERR,"Got event for frame %d but %d tile changes were indicated\n",frame_number,num_tiles_changed);
// 		}
// 	}
// 	else // we got an error reading the tile headers
// 	{
// 		corrib_syslog(LOG_ERR,"Error in %s: reading tile headers\n",__func__);
// 		return NULL;

// 	}

	cmd->bitmapDataLength = stream_get_length(s_main);
	cmd->bitmapData = stream_get_head(s_main);
	//sh_peer_calculate_average_bitrate(xfp, stream_get_length(s),numTilesChanged);

	*size = sizeof(cmd) + cmd->bitmapDataLength;
	//corrib_syslog(LOG_DEBUG,"%s: Size is %u\n",__func__,*size);
	hm_context->frame_records[hm_context->frame_record_index].number_of_tiles = num_tiles_changed;
	hm_context->frame_records[hm_context->frame_record_index].frame_size = *size;
	hm_context->frame_records[hm_context->frame_record_index].head = head;
	//corrib_syslog(LOG_DEBUG,"Assigning head to %u\n",head);
	hm_context->frame_records[hm_context->frame_record_index].processed_at=  sh_log_get_mstime();
	//corrib_syslog(LOG_DEBUG,"Adding frame record %u\n",hm_context->frame_record_index);
	hm_context->frame_record_index++;

	if(hm_context->frame_record_index > (MAX_FRAME_RECORDS -1))
	{
		hm_context->frame_record_index = 0;
	}
	return cmd;

}
