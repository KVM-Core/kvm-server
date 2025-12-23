/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RDP Server Peer
 *
 * Copyright 2011 Vic Lee
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

#ifndef FREERDP_PEER_H
#define FREERDP_PEER_H

#include <freerdp/api.h>
#include <freerdp/types.h>
#include <freerdp/settings.h>
#include <freerdp/input.h>
#include <freerdp/update.h>
#include <freerdp/autodetect.h>
#include <freerdp/redirection.h>

#include <winpr/sspi.h>
#include <winpr/ntlm.h>
#include <winpr/winsock.h>
#include <winpr/secapi.h>

// Black Box (begin)
#include <freerdp/core_event.h>
#include <freerdp/connection_manager.h>
#include <freerdp/hardware_manager.h>
#include <textfields.h>
// Black Box (end)

#ifdef __cplusplus
extern "C"
{
#endif

	typedef BOOL (*psPeerContextNew)(freerdp_peer* peer, rdpContext* context);
	typedef void (*psPeerContextFree)(freerdp_peer* peer, rdpContext* context);

	typedef BOOL (*psPeerInitialize)(freerdp_peer* peer);
#if defined(WITH_FREERDP_DEPRECATED)
	WINPR_DEPRECATED_VAR("Use psPeerGetEventHandle instead",
	                     typedef BOOL (*psPeerGetFileDescriptor)(freerdp_peer* peer, void** rfds,
	                                                             int* rcount);)
#endif
	typedef HANDLE (*psPeerGetEventHandle)(freerdp_peer* peer);
	typedef DWORD (*psPeerGetEventHandles)(freerdp_peer* peer, HANDLE* events, DWORD count);
	typedef HANDLE (*psPeerGetReceiveEventHandle)(freerdp_peer* peer);
	typedef BOOL (*psPeerCheckFileDescriptor)(freerdp_peer* peer);
	typedef BOOL (*psPeerIsWriteBlocked)(freerdp_peer* peer);
	typedef int (*psPeerDrainOutputBuffer)(freerdp_peer* peer);
	typedef BOOL (*psPeerHasMoreToRead)(freerdp_peer* peer);
	typedef BOOL (*psPeerClose)(freerdp_peer* peer);
	typedef void (*psPeerDisconnect)(freerdp_peer* peer);

	/** callback called when we receive remote credential guard credentials during NLA
	 * @param peer the associated freerdp_peer
	 * @param logonCreds the KERB_TICKET_LOGON containing the TGT and the host service ticket
	 * @param suppCreds some MSV1_0_REMOTE_SUPPLEMENTAL_CREDENTIAL containing NTLM hashes
	 * @return if the treatment was successful
	 * @bug before 3.19.0 suppCreds were a pointer to MSV1_0_SUPPLEMENTAL_CREDENTIAL, not
	 * 		MSV1_0_REMOTE_SUPPLEMENTAL_CREDENTIAL as now
	 */
	typedef BOOL (*psPeerRemoteCredentials)(freerdp_peer* peer, KERB_TICKET_LOGON* logonCreds,
	                                        MSV1_0_REMOTE_SUPPLEMENTAL_CREDENTIAL* suppCreds);

	typedef BOOL (*psPeerCapabilities)(freerdp_peer* peer);
	typedef BOOL (*psPeerPostConnect)(freerdp_peer* peer);
	typedef BOOL (*psPeerActivate)(freerdp_peer* peer);
	typedef BOOL (*psPeerLogon)(freerdp_peer* peer, const SEC_WINNT_AUTH_IDENTITY* identity,
	                            BOOL automatic);
	typedef BOOL (*psPeerSendServerRedirection)(freerdp_peer* peer,
	                                            const rdpRedirection* redirection);
	typedef BOOL (*psPeerAdjustMonitorsLayout)(freerdp_peer* peer);
	typedef BOOL (*psPeerClientCapabilities)(freerdp_peer* peer);

	typedef BOOL (*psPeerSendChannelData)(freerdp_peer* peer, UINT16 channelId, const BYTE* data,
	                                      size_t size);
	typedef BOOL (*psPeerSendChannelPacket)(freerdp_peer* client, UINT16 channelId,
	                                        size_t totalSize, UINT32 flags, const BYTE* data,
	                                        size_t chunkSize);
	typedef BOOL (*psPeerReceiveChannelData)(freerdp_peer* peer, UINT16 channelId, const BYTE* data,
	                                         size_t size, UINT32 flags, size_t totalSize);

	typedef HANDLE (*psPeerVirtualChannelOpen)(freerdp_peer* peer, const char* name, UINT32 flags);
	typedef BOOL (*psPeerVirtualChannelClose)(freerdp_peer* peer, HANDLE hChannel);
	typedef int (*psPeerVirtualChannelRead)(freerdp_peer* peer, HANDLE hChannel, BYTE* buffer,
	                                        UINT32 length);
	typedef int (*psPeerVirtualChannelWrite)(freerdp_peer* peer, HANDLE hChannel,
	                                         const BYTE* buffer, UINT32 length);
	typedef void* (*psPeerVirtualChannelGetData)(freerdp_peer* peer, HANDLE hChannel);
	typedef int (*psPeerVirtualChannelSetData)(freerdp_peer* peer, HANDLE hChannel, void* data);
	typedef BOOL (*psPeerSetState)(freerdp_peer* peer, CONNECTION_STATE state);
	typedef BOOL (*psPeerReachedState)(freerdp_peer* peer, CONNECTION_STATE state);

	/** @brief the result of the license callback */
	typedef enum
	{
		LICENSE_CB_INTERNAL_ERROR, /** an internal error happened in the callback */
		LICENSE_CB_ABORT,          /** licensing process failed, abort the connection */
		LICENSE_CB_IN_PROGRESS,    /** incoming packet has been treated, we're waiting for further
		                              packets    to complete the workflow */
		LICENSE_CB_COMPLETED       /** the licensing workflow has completed, go to next step */
	} LicenseCallbackResult;

	typedef LicenseCallbackResult (*psPeerLicenseCallback)(freerdp_peer* peer, wStream* s);

	typedef BOOL (*psPeerSignal)(freerdp_peer* client);
	typedef BOOL (*psPeerSignalClientReady)(freerdp_peer* client, UINT32 status, BOOL preemption, UINT8 * domain_key, UINT32 session_id, char * loggedin_user);
	typedef BOOL (*psPeerSignalResChangeComplete)(freerdp_peer* client, UINT32 status, int head);
	typedef BOOL (*psPeerSignalMulticastInfo)(freerdp_peer* client, UINT32 status);
	typedef BOOL (*psPeerSignalAccessStatus)(freerdp_peer* client, ACCESS_STATUS access_status, char * username);
	typedef BOOL (*psPeerSignalRecoveryRequest)(freerdp_peer* client, int head);
	typedef BOOL (*psPeerSendCloudiumMessage)(freerdp_peer* client1, UINT32 command, freerdp_peer* client2, char * username);

	typedef BOOL (*psPeerOutputReport)(freerdp_peer* client);

	struct rdp_freerdp_peer
	{
		ALIGN64 rdpContext* context;

		ALIGN64 int sockfd;
		ALIGN64 char hostname[50];

#if defined(WITH_FREERDP_DEPRECATED)
		WINPR_DEPRECATED_VAR("Use rdpContext::update instead", ALIGN64 rdpUpdate* update;)
		WINPR_DEPRECATED_VAR("Use rdpContext::settings instead", ALIGN64 rdpSettings* settings;)
		WINPR_DEPRECATED_VAR("Use rdpContext::autodetect instead",
		                     ALIGN64 rdpAutoDetect* autodetect;)
#else
	UINT64 reservedX[3];
#endif

		ALIGN64 void* ContextExtra;
		ALIGN64 size_t ContextSize;
		ALIGN64 psPeerContextNew ContextNew;
		ALIGN64 psPeerContextFree ContextFree;

		ALIGN64 psPeerInitialize Initialize;
#if defined(WITH_FREERDP_DEPRECATED)
		WINPR_DEPRECATED_VAR("Use freerdp_peer::GetEventHandle instead",
		                     ALIGN64 psPeerGetFileDescriptor GetFileDescriptor;)
#else
	UINT64 reserved;
#endif
		ALIGN64 psPeerGetEventHandle GetEventHandle;
		ALIGN64 psPeerGetReceiveEventHandle GetReceiveEventHandle;
		ALIGN64 psPeerCheckFileDescriptor CheckFileDescriptor;
		ALIGN64 psPeerClose Close;
		ALIGN64 psPeerDisconnect Disconnect;

		ALIGN64 psPeerCapabilities Capabilities;
		ALIGN64 psPeerPostConnect PostConnect;
		ALIGN64 psPeerActivate Activate;
		ALIGN64 psPeerLogon Logon;

		ALIGN64 psPeerSendServerRedirection SendServerRedirection;

		ALIGN64 psPeerSendChannelData SendChannelData;
		ALIGN64 psPeerReceiveChannelData ReceiveChannelData;

		ALIGN64 psPeerVirtualChannelOpen VirtualChannelOpen;
		ALIGN64 psPeerVirtualChannelClose VirtualChannelClose;
		ALIGN64 psPeerVirtualChannelRead VirtualChannelRead;
		ALIGN64 psPeerVirtualChannelWrite VirtualChannelWrite;
		ALIGN64 psPeerVirtualChannelGetData VirtualChannelGetData;
		ALIGN64 psPeerVirtualChannelSetData VirtualChannelSetData;

		ALIGN64 int pId;
		ALIGN64 UINT32 ack_frame_id;
		ALIGN64 BOOL local;
		ALIGN64 BOOL connected;
		ALIGN64 BOOL activated;
		ALIGN64 BOOL authenticated;
		ALIGN64 SEC_WINNT_AUTH_IDENTITY identity;

		ALIGN64 psPeerIsWriteBlocked IsWriteBlocked;
		ALIGN64 psPeerDrainOutputBuffer DrainOutputBuffer;
		ALIGN64 psPeerHasMoreToRead HasMoreToRead;
		ALIGN64 psPeerGetEventHandles GetEventHandles;
		ALIGN64 psPeerAdjustMonitorsLayout AdjustMonitorsLayout;
		ALIGN64 psPeerClientCapabilities ClientCapabilities;
#if defined(WITH_FREERDP_DEPRECATED)
		WINPR_DEPRECATED_VAR("Use freerdp_peer::SspiNtlmHashCallback instead",
		                     ALIGN64 psPeerComputeNtlmHash ComputeNtlmHash;)
#else
	UINT64 reserved2;
#endif
		ALIGN64 psPeerLicenseCallback LicenseCallback;

		ALIGN64 psPeerSendChannelPacket SendChannelPacket;

		/**
		 * @brief SetState Function pointer allowing to manually set the state of the
		 * internal state machine.
		 *
		 * This is useful if certain parts of a RDP connection must be skipped (e.g.
		 * when replaying a RDP connection dump the authentication/negotiate parts
		 * must be skipped)
		 *
		 * \note Must be called after \b Initialize as that also modifies the state.
		 */
		ALIGN64 psPeerSetState SetState;
		ALIGN64 psPeerReachedState ReachedState;
		ALIGN64 psSspiNtlmHashCallback SspiNtlmHashCallback;
		/**
		 * @brief RemoteCredentials Function pointer that will be called when remote
		 * credentials guard are used by the peer and we receive the logonCreds (kerberos)
		 * and supplementary creds (NTLM).
		 */
		ALIGN64 psPeerRemoteCredentials RemoteCredentials;

		// Black Box (begin)
		psPeerSignal Signal;
		psPeerSignalClientReady SignalClientReady;
		psPeerSignalResChangeComplete SignalResChangeComplete;
		psPeerSignalMulticastInfo SignalMulticastInfo;
		psPeerSignalAccessStatus SignalAccessStatus;
		psPeerSignalRecoveryRequest SignalRecoveryRequest;
		psPeerSendCloudiumMessage SendCloudiumMessage; //server_peer_send_cloudium_message

		psPeerOutputReport ReadOutputReport;
		// Black Box (end)

	};

// Black Box (begin)

	typedef enum {PRIMARY_PEER, TEMPORARY_PEER} rdp_peer_type;

	typedef enum {
		PEER_CONNECTION_STATE_INIT = 0,   // Connection request received
		PEER_CONNECTION_STATE_ACCEPTED,   // Connection accepted
		PEER_CONNECTION_STATE_STARTING,   // Connection audio/video setup in progress
		PEER_CONNECTION_STATE_RUNNING,    // Connection up, audio/video active
	} rdp_peer_connection_state;

	typedef enum {
		PEER_VIDEO_STATE_ACTIVE = 1,     // normal state (video active)
		PEER_VIDEO_STATE_SYNC_LOSS,  // sync loss (video stopped)
		PEER_VIDEO_STATE_RES_CHANGE, // res-change in progress (video restarting)
	} rdp_peer_video_state;

	typedef struct freerdp_secondary_peer freerdpSecondaryPeer;
	/* Structure for list of secondary peers used to send data */
	struct freerdp_secondary_peer
	{
		int status;
		struct sockaddr_in peer_sockaddr;
		freerdp_peer* freerdp_peer;
		struct freerdp_secondary_peer * next_secondary_peer;
	};

	typedef struct blackbox_peer_context bbPeerContext;

	struct blackbox_peer_context {
		rdpInput* input;
		rdpUpdate* update;
		rdpSettings* settings;

		eqEventQueue* cm_peer_queue; //will be populated by the connection manager
		eqEventQueue* peer_cm_queue; //created by the peer


		//rdpResourceControl* resourceControl;
		rdp_peer_type peer_type;
		rdp_peer_connection_state connection_state;
		rdp_peer_video_state video_state[MAX_HEAD];

		//pthread_t thread_id;
		UINT32 ack_frame_id;
		BOOL activated;
		BOOL local;
		freerdpSecondaryPeer *secondary_peer;	// list of secondary peers
		BOOL client_ready;
		int client_ready_count;
		BOOL resolution_change_active;
		//BOOL is_controlling_peer; //the lad responsible for keyboard and mouse control, a transitory honour that passes from peer to peer

		char connection_hostname[255];
		char connection_username[255];
		CONNECTION_MODE connection_mode;
		COMPRESSION_MODE compression_mode;
		UINT32 connection_start_time;
		UINT32 connection_end_time;
		UINT32 connection_duration;
		UINT32 connection_id;
		UINT32 video_master_cid;
		UINT32 audio_master_cid;
		UINT32 video_slave_cid;
		UINT32 audio_slave_cid;
		unsigned int video_sequence_number;
		unsigned int audio_sequence_number;
		int video_channel;
		int audio_channel;
		BOOL terminating;
		BOOL is_active[MAX_HEAD];
		UINT8 domain_key[6];
		UINT32 last_rtt;
		UINT32 last_mss;
		UINT32 decoder_width[MAX_HEAD];
		UINT32 decoder_height[MAX_HEAD];
		UINT32 decoder_refresh[MAX_HEAD];
	};

// Black Box (end)

	FREERDP_API void freerdp_peer_context_free(freerdp_peer* client);

	FREERDP_API BOOL freerdp_peer_context_new(freerdp_peer* client);
	FREERDP_API BOOL freerdp_peer_context_new_ex(freerdp_peer* client, const rdpSettings* settings);

	FREERDP_API const char* freerdp_peer_os_major_type_string(freerdp_peer* client);
	FREERDP_API const char* freerdp_peer_os_minor_type_string(freerdp_peer* client);

	FREERDP_API void freerdp_peer_free(freerdp_peer* client);

	WINPR_ATTR_MALLOC(freerdp_peer_free, 1)
	FREERDP_API freerdp_peer* freerdp_peer_new(int sockfd);

	FREERDP_API BOOL freerdp_peer_set_local_and_hostname(freerdp_peer* client,
	                                                     const struct sockaddr_storage* peer_addr);


// Black Box (begin)

	FREERDP_API void freerdp_peer_signal_multicast_info(freerdp_peer* client,
														UINT32 connection_type,
														const char* multicast_ip,
														const int multicast_port);

// Black Box (end)


#ifdef __cplusplus
}
#endif

#endif /* FREERDP_PEER_H */
