/*
 * Brian Quinn
 * Copyright: Cloudium Systems 2014
 */

#include "audio_packetizer.h"
// #include "packetizer_debug.h"
#include <freerdp/hardware_manager.h>
#include <corrib_logger.h>
#include <freerdp/utils/memory.h>
//#include "statistics_json.h"
#include <restapi.h>
/*
 * takes audio fd data and reads it into an audio command packet
 * use hw_manager context to do this
 *
 */

#define WAVE_FORMAT_PCM	0x0001


/*
 * Constructor
 */
apContext*  audio_packetizer_new(void)
{
	apContext * ap_context =  xnew(apContext,__func__);
	//ap_context->data = (UINT8*) xzalloc(10000,__func__);
	if (ap_context != NULL)
	{
		ap_context->formatTag = WAVE_FORMAT_PCM;
		ap_context->channels = 2;
		ap_context->samplesPerSec = 16000;
		ap_context->blockAlign = 4;
		ap_context->bitsPerSample = 16;
		ap_context->samplesLatency = 3840/2;//3840;

		ap_context->audioData = (UINT8*) xzalloc(ap_context->samplesLatency * (ap_context->bitsPerSample / 8),__func__);
	}

	return ap_context;
}


/* Destructor
 *
 */
void audio_packetizer_free(apContext * ap_context)
{
	if (ap_context->audioData != NULL)
		xfree(ap_context->audioData,__func__);
	xfree(ap_context,__func__);
}


AUDIO_DATA_COMMAND* ap_create_audio_command(apContext* ap_context, hwManagerContext* hm_context)
{
	// STREAM * s_main;
	AUDIO_DATA_COMMAND * cmd = NULL; //Uncomment ARPMaudio_data_command_new(ap_context->audioData);
	int readSize, maxTotalSize = ap_context->samplesLatency * (ap_context->bitsPerSample / 8);
	int i,maxReadSize = (((ap_context->samplesPerSec / 1000) * ap_context->bitsPerSample) / 8) * ap_context->channels;

	//corrib_syslog(LOG_DEBUG, "ap_create_audio_command maxReadSize is %d", maxReadSize);

	do
	{
		readSize = read(hm_context->audio_fd, cmd->audioData + cmd->audioDataLength, maxReadSize);
		if (readSize < 0) // we got an error reading the audio fd
		{
	    	if(!((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
	    		 (errno == EINPROGRESS)))
	    	{
	    		corrib_syslog(LOG_ERR,"Error in %s: reading audio fd\n",__func__);
	    		//audio_data_command_free(cmd);
	    		//return NULL;
	    	}
	    	break;
		}
		//corrib_syslog(LOG_DEBUG, "[ ");
		//for(i=0;i<16;i++)
		//{
		//	corrib_syslog(LOG_DEBUG, "%02x", cmd->audioData[cmd->audioDataLength + i]);
		//}
		//corrib_syslog(LOG_DEBUG, "]\n");
		//corrib_syslog(LOG_DEBUG, " readSize is %d ", readSize);

		cmd->audioDataLength += readSize;
	}
	while (readSize && cmd->audioDataLength < maxTotalSize);

	if(cmd)
	{
		//corrib_syslog(LOG_DEBUG, "[ audioDataLength is %d ]", cmd->audioDataLength);

		if (cmd->audioDataLength)
		{
			UINT32 current_time;
			UINT32 time_difference;
			current_time=sh_log_get_mstime();
// ARPM Uncomment 			hm_context->UsbaudioStatistics.total_audio_frames_count++;
// 			hm_context->UsbaudioStatistics.total_audio_bytes_sent+=cmd->audioDataLength;
// 			time_difference = current_time - hm_context->UsbaudioStatistics.previous_time_audio;
// 			if(time_difference > 60000) // more than a minute has elapsed
// 			{
// 				hm_context->UsbaudioStatistics.audio_bytes_per_second = (float)((float)(hm_context->UsbaudioStatistics.total_audio_bytes_sent/60));
// 				if(hm_context->UsbaudioStatistics.moving_average_audio == 0)
// 				{
// 					hm_context->UsbaudioStatistics.moving_average_audio = hm_context->UsbaudioStatistics.audio_bytes_per_second;
// 					corrib_syslog(LOG_DEBUG,"resetting avg=%f   \t",hm_context->UsbaudioStatistics.moving_average_audio);
// 				}
// 				hm_context->UsbaudioStatistics.moving_average_audio = hw_manager_get_rolling_average(hm_context->UsbaudioStatistics.moving_average_audio,hm_context->UsbaudioStatistics.audio_bytes_per_second,1);
// 				if(hm_context->UsbaudioStatistics.audio_bytes_per_second < hm_context->UsbaudioStatistics.min_audio_bytes_per_second )
// 					hm_context->UsbaudioStatistics.min_audio_bytes_per_second = hm_context->UsbaudioStatistics.audio_bytes_per_second;
// 				if(hm_context->UsbaudioStatistics.audio_bytes_per_second > hm_context->UsbaudioStatistics.max_audio_bytes_per_second )
// 					hm_context->UsbaudioStatistics.max_audio_bytes_per_second = hm_context->UsbaudioStatistics.audio_bytes_per_second;
// #ifdef ENABLE_WEB_SERVICE_REPORTING
// 				statistcs_send_json_usb_audio_fact_object(&(hm_context->UsbaudioStatistics));
// #endif
// 				hm_context->UsbaudioStatistics.total_audio_bytes_sent = 0;
// 				hm_context->UsbaudioStatistics.previous_time_audio = current_time;
// 				//corrib_syslog(LOG_DEBUG,"updated ct=%u pt=%u\n",current_time,hm_context->UsbaudioStatistics.previous_time_audio);
// 			}
// 			else
// 			{
// 				//corrib_syslog(LOG_DEBUG,"skipped ct=%u pt=%u d=%u\n",current_time,hm_context->UsbaudioStatistics.previous_time_audio,time_difference);
// 			}

			cmd->formatTag = ap_context->formatTag;
			cmd->channels = ap_context->channels;
			cmd->samplesPerSec = ap_context->samplesPerSec;
			cmd->blockAlign = ap_context->blockAlign;
			cmd->bitsPerSample = ap_context->bitsPerSample;


		}
		else
		{
			//August 2016: In discussion with Brian, we do not believe this is an error,
			//as per design we may end up reading all available audio on first trip to find no audio on subsequent trip.
			//corrib_syslog(LOG_ERR,"%s: Got audio event but no audio data was available\n",__func__);
			// ARPM uncomment audio_data_command_free(cmd);
			cmd = NULL;
		}

	}
	return cmd;
}
