#ifndef __AUDIO_PACKETIZER_H
#define __AUDIO_PACKETIZER_H

#include <freerdp/update.h>
#include <freerdp/hardware_manager.h>

typedef struct audio_packetizer_context apContext;

struct audio_packetizer_context
{
	UINT16 formatTag;
	UINT16 channels;
	UINT32 samplesPerSec;
	UINT16 blockAlign;
	UINT16 bitsPerSample;
	UINT16 samplesLatency;
	UINT8 *audioData;
};

apContext*  audio_packetizer_new();
void audio_packetizer_free(apContext* ap_context);
AUDIO_DATA_COMMAND* ap_create_audio_command(apContext* ap_context, hwManagerContext* hm_context);

#endif //__AUDIO_PACKETIZER_H
