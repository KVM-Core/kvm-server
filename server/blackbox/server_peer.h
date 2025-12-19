/**
 * FreeRDP: A Remote Desktop Protocol Client
 * X11 Peer
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SERVER_PEER_H
#define __SERVER_PEER_H

#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/region.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/listener.h>
// #include <freerdp/utils/stream.h>
#include <freerdp/utils/stopwatch.h>
#include <time.h>
#include <freerdp/types.h>
#include <freerdp/utils/event_queue.h>

typedef struct server_peer_context serverPeerContext;

#include "bbfreerdp.h"
#include <freerdp/utils/sh_logger.h>
#include <freerdp/utils/profiler.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/rdpsnd.h>
// #include <freerdp/server/rdpeusb.h>


#define DEFAULT_INTERVAL_PERIOD  1
#define WAVE_FORMAT_PCM	0x0001


struct server_peer_context
{
	rdpContext _p;

	freerdp_peer * client;

	UINT32 fps;


	// WTSVirtualChannelManager* vcm;
	rdpsnd_server_context* rdpsnd;
	// rdpeusbServerContext* rdpeusb;

	//------------------------------- Peer Status -------------------
	BOOL activated;


	//---------------------------------Queues-----------------------
	//These are populated by the connection manager when the context is created
	eqEventQueue* cm_peer_queue; //allocated by the CM when the peer is created
	eqEventQueue* peer_cm_queue; //allocated by the CM when the peer is created
	eqEventQueue* sp_sp_queue; //allocated by the Server Peer, acts as a inter thread queue for any threads within the server peer
	eqEventQueue* sp_mon_queue; //allocated by the Server Peer, acts as a inter thread queue for the monitor thread within the server peer
	//-----------------------------------Threads----------------------
	//-------------------------------------------------------------
	pthread_t main_thread;
	pthread_t secondary_thread;
	pthread_t monitor_thread;
	threadState main_thread_state;
	threadState secondary_thread_state;
	threadState monitor_thread_state;
	BOOL main_thread_running;
	BOOL secondary_thread_running;
	BOOL monitor_thread_running;

	BOOL frame_processing_h1; //true if we have been told a frame is available, false on we have completed delivery, if its true then we may have gotten a frame but not delivered
	BOOL frame_processing_h2; //true if we have been told a frame is available, false on we have completed delivery, if its true then we may have gotten a frame but not delivered


	unsigned int surface_commands_is_use;


	pthread_mutex_t rdpsnd_mutex;
};

void server_peer_accepted(cmContext* cm_context, freerdp_peer* client);

#endif /* __SERVER_PEER_H */
