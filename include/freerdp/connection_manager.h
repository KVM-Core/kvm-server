/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 *
 * Copyright 2025 Black Box
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

#ifndef __CONNECTION_MANAGER_H
#define __CONNECTION_MANAGER_H

#include <sys/socket.h>
#include <linux/netlink.h>
#include <freerdp/peer.h>
#include <freerdp/api.h>
#include <freerdp/types.h>
#include <freerdp/settings.h>
#include <freerdp/peer.h>
#include <freerdp/utils/event_queue.h>
#include <freerdp/utils/queue.h>
#include <freerdp/hardware_manager.h>
#include <textfields.h>

//#define SHARED_MODE_DEBUG
#define MAX_CLOUMNS 2	//[cid value][availability
#define MAX_SHARED_CONNECTIONS		8
#define VIDEO_MASTER_CID  		0x12
#define AUDIO_MASTER_CID  		0x1B
#define STARTING_MULTICAST_VIDEO_CID  	0x13
#define STARTING_MULTICAST_AUDIO_CID  	0x1C
#define STARTING_MUNICAST_VIDEO_CID  	0x02
#define STARTING_MUNICAST_AUDIO_CID  	0x0A

#define CM_DEFAULT_INTERVAL_PERIOD 1

/* Netlink socket */
#define NETLINK_USER 31
#define NLINK_MSG_LEN 1024

struct nl_message {
	int cid;
	int event_id;
};

typedef struct connection_manager_context cmContext;

// typedef void (*cmPeerAccepted)(cmContext * cm_context, freerdp_peer* client);
// typedef void (*cmMulticastPeerAccepted)(cmContext * cm_context, m_peer* client);

typedef struct peer_node peerNode;

typedef struct peer_wait_node peerWaitNode;

//typedef enum {UNICAST, MULTICAST, MUNICAST} operating_mode;
typedef enum {VIDEO_MODE, AUDIO_MODE, AV_MODE} av_mode;

struct peer_node
{
	freerdp_peer* client;
	int id;
	LIST_ENTRY(peer_node) entries;      /* List. */
};

struct peer_wait_node
{
	int id;
	char hostname[50]; //the hostname of the connection
	int fd;
	TAILQ_ENTRY(peer_wait_node) entries; /* Queue. */
};

typedef struct connection_manager_context cmContext;

struct connection_manager_context
{
	
	//-------------------------References------------------------
	//-----------------------------------------------------------
	hwManagerContext * hm_context;

	//--------------------------Callbacks-------------------------
	//-------------------------------------------------------------
	// cmPeerAccepted PeerAccepted; //we call this to initialise the peer
	// cmMulticastPeerAccepted mPeerAccepted; //we call this to initialise the mpeer

	//---------------------- Peer List ------------------------------
	//---------------------------------------------------------------
	LIST_HEAD(peer_list, peer_node) peer_list_head;
	TAILQ_HEAD(peer_wait_queue, peer_wait_node) peer_wait_queue_head;

	//---------------------------------Queues-----------------------
	//-------------------------------------------------------------
	eqEventQueue* hm_cm_queue; //input queue from hardware manager
	eqEventQueue* cm_hm_queue; //output queue to hardware manager
	eqEventQueue* listener_queue; //input queue from the listner, used to communicate new connections and end events
	eqEventQueue* peer_cm_queue; //input queue from the various, all peers can write to this queue

	//-----------------------------------Threads----------------------
	//-------------------------------------------------------------
	pthread_t main_thread;
	// threadState main_thread_state;
	//----------------------------------- status ----------------------
	//-------------------------------------------------------------
	// exitState hm_exit_state;
	char exit_info[255];
	UINT32 client_id_counter;

	//------------------------------------ Debug and Profiling ----------------
	//-------------------------------------------------------------------------
	BOOL performance_analysis;
	BOOL debug_enabled;
	// esContext * es_context;

	//--------------------------------------Statistics--------------------
	//--------------------------------------------------------------------
	UINT32 last_surface_command_available_h1;
	UINT32 last_surface_command_available_h2;
	UINT32 last_audio_command_available;
	UINT32 last_mouse_event;

	UINT32 succesful_logins;
	UINT32 failed_logins;
	UINT32 previous_interval_time;
	UINT32 connection_id_pool;
	UINT32 statistics_counter;

	//-------------------------------------- Locking -------------------------------
	//------------------------------------------------------------------------------
	pthread_mutex_t mutex;


	//--------------------------------------Multicast ------------------------------
	//------------------------------------------------------------------------------
	CONNECTION_MODE cm_operating_mode; //Indicates the type of connection we are supporting unicast, multicast etc
	COMPRESSION_MODE cm_compression_mode;
	SERVER_TECHNOLOGY_TYPE server_technology_type;
	BOOL preemption;
	// m_peer * multicast_peer_client;
	UINT32 mouse_keyboard_timer;
	BOOL mouse_keyboard_available;
	int controlling_peer_id;
	UINT32 video_slave_cid_pool[MAX_SHARED_CONNECTIONS][MAX_CLOUMNS]; // [cid value][availability]
	UINT32 audio_slave_cid_pool[MAX_SHARED_CONNECTIONS][MAX_CLOUMNS]; // [cid value][availability]

	BOOL video_municast_running;
	BOOL audio_municast_running;
	BOOL video_multicast_running;
	BOOL audio_multicast_running;
	int multicast_last_rtt;
	int audio_master_mu_channel;
	freerdp_peer * connecting_client;
	//-------------------------------------Multi Unicast----------------------------
	//------------------------------------------------------------------------------

	UINT32 video_channels[MAX_SHARED_CONNECTIONS];
	UINT32 audio_channels[MAX_SHARED_CONNECTIONS];
	BOOL resolution_change_needed[MAX_HEAD];

	//------------------------------------- Unicast----------------------------
	//------------------------------------------------------------------------------
	char loggedin_user[32];
	UINT32 mouse_keyboard_timeout;
    BOOL enable_cm_heartbeats;

	//------------------------------------- Netlink----------------------------
	//------------------------------------------------------------------------------
	int netlink_sock_fd;
};

cmContext *  connection_manager_new(void);
void connection_manager_free(cmContext * cm_context);
void connection_manager_set_queues(cmContext * cm_context,eqEventQueue* listner_queue);
void connection_manager_run(cmContext * cm_context);
#if 0

#include <sys/socket.h>
#include <linux/netlink.h>
#include <freerdp/hardware_manager.h>
#include <freerdp/peer.h>
#include <freerdp/api.h>
#include <freerdp/types.h>
#include <freerdp/settings.h>
#include <freerdp/peer.h>
#include <freerdp/utils/queue.h>
#include <freerdp/utils/event_sender.h>
#include <textfields.h>

//#define SHARED_MODE_DEBUG
#define MAX_CLOUMNS 2	//[cid value][availability
#define MAX_SHARED_CONNECTIONS		8
#define VIDEO_MASTER_CID  		0x12
#define AUDIO_MASTER_CID  		0x1B
#define STARTING_MULTICAST_VIDEO_CID  	0x13
#define STARTING_MULTICAST_AUDIO_CID  	0x1C
#define STARTING_MUNICAST_VIDEO_CID  	0x02
#define STARTING_MUNICAST_AUDIO_CID  	0x0A

#define CM_DEFAULT_INTERVAL_PERIOD 1

/* Netlink socket */
#define NETLINK_USER 31
#define NLINK_MSG_LEN 1024
struct nl_message {
	int cid;
	int event_id;
};

typedef struct connection_manager_context cmContext;

typedef void (*cmPeerAccepted)(cmContext * cm_context, freerdp_peer* client);
typedef void (*cmMulticastPeerAccepted)(cmContext * cm_context, m_peer* client);

typedef struct peer_node peerNode;

typedef struct peer_wait_node peerWaitNode;

//typedef enum {UNICAST, MULTICAST, MUNICAST} operating_mode;
typedef enum {VIDEO_MODE, AUDIO_MODE, AV_MODE} av_mode;

struct peer_node
{
	freerdp_peer* client;
	int id;
	LIST_ENTRY(peer_node) entries;      /* List. */
};

struct peer_wait_node
{
	int id;
	char hostname[50]; //the hostname of the connection
	int fd;
	TAILQ_ENTRY(peer_wait_node) entries; /* Queue. */
};

struct connection_manager_context
{

	//-------------------------References------------------------
	//-----------------------------------------------------------
	hwManagerContext * hm_context;

	//--------------------------Callbacks-------------------------
	//-------------------------------------------------------------
	cmPeerAccepted PeerAccepted; //we call this to initialise the peer
	cmMulticastPeerAccepted mPeerAccepted; //we call this to initialise the mpeer

	//---------------------- Peer List ------------------------------
	//---------------------------------------------------------------
	LIST_HEAD(peer_list, peer_node) peer_list_head;
	TAILQ_HEAD(peer_wait_queue, peer_wait_node) peer_wait_queue_head;

	//---------------------------------Queues-----------------------
	//-------------------------------------------------------------
	eqEventQueue* hm_cm_queue; //input queue from hardware manager
	eqEventQueue* cm_hm_queue; //output queue to hardware manager
	eqEventQueue* listner_queue; //input queue from the listner, used to communicate new connections and end events
	eqEventQueue* peer_cm_queue; //input queue from the various, all peers can write to this queue

	//-----------------------------------Threads----------------------
	//-------------------------------------------------------------
	pthread_t main_thread;
	threadState main_thread_state;
	//----------------------------------- status ----------------------
	//-------------------------------------------------------------
	exitState hm_exit_state;
	char exit_info[255];
	uint32 client_id_counter;

	//------------------------------------ Debug and Profiling ----------------
	//-------------------------------------------------------------------------
	boolean performance_analysis;
	boolean debug_enabled;
	esContext * es_context;

	//--------------------------------------Statistics--------------------
	//--------------------------------------------------------------------
	uint32 last_surface_command_available_h1;
	uint32 last_surface_command_available_h2;
	uint32 last_audio_command_available;
	uint32 last_mouse_event;

	uint32 succesful_logins;
	uint32 failed_logins;
	uint32 previous_interval_time;
	uint32 connection_id_pool;
	uint32 statistics_counter;

	//-------------------------------------- Locking -------------------------------
	//------------------------------------------------------------------------------
	pthread_mutex_t mutex;


	//--------------------------------------Multicast ------------------------------
	//------------------------------------------------------------------------------
	CONNECTION_MODE cm_operating_mode; //Indicates the type of connection we are supporting unicast, multicast etc
	COMPRESSION_MODE cm_compression_mode;
	SERVER_TECHNOLOGY_TYPE server_technology_type;
	boolean preemption;
	m_peer * multicast_peer_client;
	uint32 mouse_keyboard_timer;
	boolean mouse_keyboard_available;
	int controlling_peer_id;
	uint32 video_slave_cid_pool[MAX_SHARED_CONNECTIONS][MAX_CLOUMNS]; // [cid value][availability]
	uint32 audio_slave_cid_pool[MAX_SHARED_CONNECTIONS][MAX_CLOUMNS]; // [cid value][availability]

	boolean video_municast_running;
	boolean audio_municast_running;
	boolean video_multicast_running;
	boolean audio_multicast_running;
	int multicast_last_rtt;
	int audio_master_mu_channel;
	freerdp_peer * connecting_client;
	//-------------------------------------Multi Unicast----------------------------
	//------------------------------------------------------------------------------

	uint32 video_channels[MAX_SHARED_CONNECTIONS];
	uint32 audio_channels[MAX_SHARED_CONNECTIONS];
	boolean resolution_change_needed[MAX_HEAD];

	//------------------------------------- Unicast----------------------------
	//------------------------------------------------------------------------------
	char loggedin_user[32];
	uint32 mouse_keyboard_timeout;
        boolean enable_cm_heartbeats;

	//------------------------------------- Netlink----------------------------
	//------------------------------------------------------------------------------
	int netlink_sock_fd;
};

cmContext *  connection_manager_new();
void connection_manager_free(cmContext * cm_context);
void connection_manager_set_queues(cmContext * cm_context,eqEventQueue* listner_queue);
void connection_manager_run(cmContext * cm_context);
void connection_manager_enable_performance_analysis(cmContext * cm_context);
void connection_manager_clear_peer_list();
void connection_manager_enable_debug(cmContext * cm_context);
void connection_manager_set_default_trace_target(cmContext * cm_context,char * target);
unsigned int connection_manager_get_slave_cid(cmContext * cm_context,av_mode mode, freerdp_peer * client);
boolean connection_manager_fastpath_tcp_init(cmContext *cm_context, freerdp_peer *client);
int connect_manager_get_peer_list_size(cmContext * cm_context);
int connect_manager_get_peer_list_size_and_types(cmContext * cm_context, uint32_t * optimised_peers, uint32_t * lossless_peers);
void connection_manager_populate_client_resolution_values(hwManagerContext * hw_context, const COMPRESSION_MODE compression, rdpSettings * settings, const video_head_index_e head);
void connection_manager_set_server_mode(cmContext * cm_context, freerdp_peer * client);

#endif
#endif //__CONNECTION_MANAGER_H
