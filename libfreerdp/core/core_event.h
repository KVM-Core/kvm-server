
#ifndef __CORE_EVENT_H
#define __CORE_EVENT_H

#include <freerdp/constants.h>
#include <freerdp/types.h>
#include <freerdp/codec/rfx.h>
// #include <freerdp/channels/rdpeusb.h>
#include <freerdp/freerdp.h>
#include <freerdp/utils/event_queue.h>
#include "mcs.h"


enum access_status {
	ACCESS_STATUS_GRANTED,
	ACCESS_STATUS_DENIED
};
typedef enum access_status ACCESS_STATUS;


//-------- End Event ----------------------------------
typedef struct event_end EventEnd;
struct event_end
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members

};
EventEnd * event_end_new();
void event_end_free(EventEnd* event_end);
//------------------------------------------


//-------- Peer Terminated Event ----------------------------------
typedef struct event_peer_terminated EventPeerTerminated;
struct event_peer_terminated
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	int peer_id;
	int peer_cid;
	BOOL frame_processing_h1;
	BOOL frame_processing_h2;
	//maybe add a termination reason here
	//------ all events must start with these members

};
EventPeerTerminated* event_peer_terminated_new(int peer_id,int peer_cid);
void event_peer_terminated_free(EventPeerTerminated* event_peer_terminated);
//------------------------------------------



//-------- Input Event ----------------------------------
// sent from the mouse thread to the main thread
typedef struct event_input EventInput;
struct event_input
{
	int type; //the type of event
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT16 input_type;
	UINT16 flags;
	//int x;
	//int y;
	UINT32 x;
	UINT32 y;
	UINT32 key_id;
	UINT32 key_symbol;
	UINT32 button;
	UINT32 axisrel;
	UINT32 previous_frame_receive_time;


};
EventInput* event_input_new(int x,int y,UINT16 flags,UINT16 input_type);
void event_input_free(EventInput* event_input);
void event_input_show(EventInput* event_input);
void event_input_json_serialise(EventInput* event_input, char * buffer,int  buffer_size);
//------------------------------------------



//-------- Mouse Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_mouse EventMouse;
struct event_mouse
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT16 flags;
	//int x;
	//int y;
	UINT32 x;
	UINT32 y;
	int peer_id; //where the event originated
	UINT32 previous_frame_receive_time;


};
EventMouse* event_mouse_new(int x,int y,UINT16 flags,int peer_id);
void event_mouse_free(EventMouse* event_mouse);
void event_mouse_show(EventMouse* event_mouse);
void event_mouse_json_serialise(EventMouse* event_mouse, char * buffer,int  buffer_size);
//------------------------------------------


//-------- Keyboard Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_keyboard EventKeyboard;
struct event_keyboard
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT16 flags;
	UINT16 code;
	int peer_id; //where the event originated


};
EventKeyboard* event_keyboard_new(UINT16 code,UINT16 flags,int peer_id);
void event_keyboard_free(EventKeyboard* event_keyboard);
void event_keyboard_show(EventKeyboard* event_keyboard);
void event_keyboard_json_serialise(EventKeyboard* event_keyboard, char * buffer,int  buffer_size);
//------------------------------------------

//-------- KeyboardOutputReport Event ----------------------------------
// sent from the hardware manager to the connection manager
typedef struct event_keyboard_output_report EventKeyboardOutputReport;
struct event_keyboard_output_report
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT8 mask;

};
EventKeyboardOutputReport* event_keyboard_output_report_new(UINT8 mask);
void event_keyboard_output_report_free(EventKeyboardOutputReport* event_keyboard_output_report);
void event_keyboard_output_report_show(EventKeyboardOutputReport* event_keyboard_output_report);
void event_keyboard_output_report_json_serialise(EventKeyboardOutputReport* event_keyboard_output_report, char * buffer,int  buffer_size);
//------------------------------------------


//-------- resolution_change Event ----------------------------------
// sent to the connection manager from the hardware manager
typedef struct event_resolution_change EventResolutionChange;
struct event_resolution_change
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id; //where the event originated head wise
	// videoData_t connectionResData;
    UINT32 connection_type;
};

// EventResolutionChange* event_resolution_change_new(int head_id, const videoData_t connectionResData);
void event_resolution_change_free(EventResolutionChange* event_resolution_change);
void event_resolution_change_show(EventResolutionChange* event_resolution_change);
void event_resolution_change_json_serialise(EventResolutionChange* event_resolution_change, char * buffer,int  buffer_size);
//------------------------------------------

//-------- Cloudium Message Event ----------------------------------
// sent to the connection manager from the hardware manager
typedef struct event_cloudium_message EventCloudiumMessage;
struct event_cloudium_message
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	freerdp_peer* client1;
	UINT32 command;
	freerdp_peer* client2;
	char username[255];
	UINT32 preemption_time;


};
EventCloudiumMessage* event_cloudium_message_new(freerdp_peer* client1,UINT32 command,freerdp_peer* client2,char * username,UINT32 preemption_time);
void event_cloudium_message_free(EventCloudiumMessage* event_cloudium_message);
void event_cloudium_message_show(EventCloudiumMessage* event_cloudium_message);
void event_cloudium_message_json_serialise(EventCloudiumMessage* event_cloudium_message, char * buffer,int  buffer_size);
//------------------------------------------







//-------- sync_loss Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_sync_loss EventSyncLoss;
struct event_sync_loss
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id; //where the event originated head wise


};
EventSyncLoss* event_sync_loss_new(int head_id);
void event_sync_loss_free(EventSyncLoss* event_sync_loss);
void event_sync_loss_show(EventSyncLoss* event_sync_loss);
void event_sync_loss_json_serialise(EventSyncLoss* event_sync_loss, char * buffer,int  buffer_size);
void event_resolution_change_json_serialise(EventResolutionChange* event_resolution_change, char * buffer,int  buffer_size);
//------------------------------------------


//-------- new_connection Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_new_connection EventNewConnection;
struct event_new_connection
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int peer_sockfd; // Socket FD associated with the peer as determined by the lisner
	char hostname[50]; //the hostname of the connection


};
EventNewConnection* event_new_connection_new(int peer_id,const char * hostname);
void event_new_connection_free(EventNewConnection* event_new_connection);
void event_new_connection_show(EventNewConnection* event_new_connection);
void event_new_connection_json_serialise(EventNewConnection* event_new_connection, char * buffer,int  buffer_size);
//------------------------------------------



//-------- connection_status Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_connection_status EventConnectionStatus;
struct event_connection_status
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int peer_sockfd; // Socket FD associated with the peer as determined by the lisner
	char hostname[50]; //the hostname of the connection


};
EventConnectionStatus* event_connection_status_new(int peer_id,const char * hostname);
void event_connection_status_free(EventConnectionStatus* event_connection_status);
void event_connection_status_show(EventConnectionStatus* event_connection_status);
void event_connection_status_json_serialise(EventConnectionStatus* event_connection_status, char * buffer,int  buffer_size);
//------------------------------------------


//--------Decode Done Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_decode_done EventEncodeDone;
struct event_decode_done
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_receive_time;
	int size;
	int head_id;
	// FRAME_TYPES frame_type;
	int frame_number;
	int starting_tile;
	int number_of_tiles;

};
// EventEncodeDone* event_encode_done_new(int head_id,FRAME_TYPES frame_type,int frame_number,int starting_tile,int number_of_tiles);
void event_encode_done_free(EventEncodeDone* event_decode_done);
void event_encode_done_show(EventEncodeDone* event_decode_done);
void event_encode_done_json_serialise(EventEncodeDone* event_decode_done, char * buffer,int  buffer_size);
void event_encode_done_video_purge( eqEventQueue* event_queue, int head);
//------------------------------------------


//--------Audio Done Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_audio_done EventAudioDone;
struct event_audio_done
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_receive_time;

};
EventAudioDone* event_audio_done_new(void);
void event_audio_done_free(EventAudioDone* event_audio_done);
void event_audio_done_show(EventAudioDone* event_audio_done);
void event_audio_done_json_serialise(EventAudioDone* event_audio_done, char * buffer,int  buffer_size);
//------------------------------------------


//-------- Virtual Done Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_virtual_done EventVirtualDone;
struct event_virtual_done
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_receive_time;
	// virtualDevice *device;
};

// EventVirtualDone* event_virtual_done_new(virtualDevice *device);
void event_virtual_done_free(EventVirtualDone* event_virtual_done);
void event_virtual_done_show(EventVirtualDone* event_virtual_done);
void event_virtual_done_json_serialise(EventVirtualDone* event_virtual_done, char * buffer,int  buffer_size);
//------------------------------------------



//--------Surface Command Available Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_surface_command_available EventSurfaceCommandAvailable;
struct event_surface_command_available
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_receive_time;
	void * cmd;
	int size;
	int head_id;
	// FRAME_TYPES frame_type;
	int frame_number;
	int number_of_tiles;
	int dropped_frames;
	UINT32 frames_in_flight;
	BOOL is_duplicate;
	UINT32 encode_done_timestamp;
	UINT32 frame_size;
	char peer_ip[50];
	int command_id;

};
// EventSurfaceCommandAvailable* event_surface_command_available_new(int head_id,void * cmd,FRAME_TYPES frame_type,int frame_number,int number_of_tiles,int dropped_frames,UINT32 frame_size);
EventSurfaceCommandAvailable* event_surface_command_available_clone(EventSurfaceCommandAvailable * event);
void event_surface_command_available_free(EventSurfaceCommandAvailable* event_surface_command_available);
void event_surface_command_available_show(EventSurfaceCommandAvailable* event_surface_command_available);
void event_surface_command_available_json_serialise(EventSurfaceCommandAvailable* event_surface_command_available, char * buffer,int  buffer_size);
//------------------------------------------


//--------Surface Command Available Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_audio_command_available EventAudioCommandAvailable;
struct event_audio_command_available
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_receive_time;
	void * cmd;
	int size;

};
EventAudioCommandAvailable* event_audio_command_available_new(void * cmd);
void event_audio_command_available_free(EventAudioCommandAvailable* event_audio_command_available);
void event_audio_command_available_show(EventAudioCommandAvailable* event_audio_command_available);
void event_audio_command_available_json_serialise(EventAudioCommandAvailable* event_audio_command_available, char * buffer,int  buffer_size);
//------------------------------------------


//-------- Channel Ready Event ----------------------------------
// sent from the server peer to the connection manager
typedef struct event_channel_ready EventChannelReady;
struct event_channel_ready
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	rdpMcsChannel* channel;
};

EventChannelReady* event_channel_ready_new(rdpMcsChannel* channel);
void event_channel_ready_free(EventChannelReady* event_channel_ready);
void event_channel_ready_show(EventChannelReady* event_channel_ready);
void event_channel_ready_json_serialise(EventChannelReady* event_channel_ready, char * buffer,int  buffer_size);
//------------------------------------------


//--------RdpeUsb Command Available Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_usb_command_available EventUsbCommandAvailable;
struct event_usb_command_available
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	// sequenceData* sequence_data;
	int device_id;
	void * cmd;
	int size;

};

// EventUsbCommandAvailable* event_usb_command_available_new(sequenceData * sequence_data, int device_id, void * cmd);
void event_usb_command_available_free(EventUsbCommandAvailable* event_rdpeusb_command_available);
void event_usb_command_available_show(EventUsbCommandAvailable* event_rdpeusb_command_available);
void event_usb_command_available_json_serialise(EventUsbCommandAvailable* event_rdpeusb_command_available, char * buffer,int  buffer_size);
//------------------------------------------



//--------Surface Command Sent Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_surface_command_sent EventSurfaceCommandSent;
struct event_surface_command_sent
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id;
	// FRAME_TYPES frame_type;
	int frame_number;
	void * cmd;
	int number_of_tiles;
	UINT32 command_available_time;

};
// EventSurfaceCommandSent* event_surface_command_sent_new(int head_id,FRAME_TYPES frame_type,int frame_number,void * cmd);
void event_surface_command_sent_free(EventSurfaceCommandSent* event_surface_command_sent);
void event_surface_command_sent_show(EventSurfaceCommandSent* event_surface_command_sent);
void event_surface_command_sent_json_serialise(EventSurfaceCommandSent* event_surface_command_sent, char * buffer,int  buffer_size);
//------------------------------------------

//--------Peer Access Status Event ----------------------------------
//
typedef struct event_peer_access_status EventAccessStatus;
struct event_peer_access_status
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int peer_id;
	ACCESS_STATUS access_status;
	// char username[MAX_USERNAME_LENGTH];


};
EventAccessStatus* event_peer_access_status_new(int peer_id,ACCESS_STATUS status);
void event_peer_access_status_free(EventAccessStatus* event_peer_access_status);
void event_peer_access_status_show(EventAccessStatus* event_peer_access_status);
void event_peer_access_status_json_serialise(EventAccessStatus* event_peer_access_status, char * buffer,int  buffer_size);
//------------------------------------------



//--------Peer Access Status Event ----------------------------------
//
typedef struct event_peer_recovery_request EventRecoveryRequest;
struct event_peer_recovery_request
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id;



};
EventRecoveryRequest* event_peer_recovery_request_new(int peer_id,int head);
void event_peer_recovery_request_free(EventRecoveryRequest* event_peer_recovery_request);
void event_peer_recovery_request_show(EventRecoveryRequest* event_peer_recovery_request);
void event_peer_recovery_request_json_serialise(EventRecoveryRequest* event_peer_recovery_request, char * buffer,int  buffer_size);
//------------------------------------------



//--------Surface Command Complete Event ----------------------------------
// complete from the connection manager to the hardware manager
typedef struct event_surface_command_complete EventSurfaceCommandComplete;
struct event_surface_command_complete
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id;
	int frame_number;
	void * cmd;
	UINT32 command_available_time;
	UINT32 command_sent_time;
	UINT32 encode_done_time;
	UINT32 previous_frame_time;
	UINT32 frame_size;
	UINT32 dropped_frames;
	UINT32 number_of_tiles;
	int command_id;
	UINT32 connection_id;


};
EventSurfaceCommandComplete* event_surface_command_complete_new(int head_id,int frame_number,void * cmd);
EventSurfaceCommandComplete* event_surface_command_complete_clone_from_sent_command(EventSurfaceCommandSent *  event_surface_command_sent);
EventSurfaceCommandComplete* event_surface_command_complete_clone_from_available_command(EventSurfaceCommandAvailable *  event_surface_command_available);
void event_surface_command_complete_free(EventSurfaceCommandComplete* event_surface_command_complete);
void event_surface_command_complete_show(EventSurfaceCommandComplete* event_surface_command_complete);
void event_surface_command_complete_json_serialise(EventSurfaceCommandComplete* event_surface_command_complete, char * buffer,int  buffer_size);
//------------------------------------------



//-------- desktop_resize Event ----------------------------------
// sent from the connection manager to the hardware manager
typedef struct event_desktop_resize EventDesktopResize;
struct event_desktop_resize
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int head_id; //not sure of the significance of the head for this event
	int sync_loss;
	int width;
	int height;
    int refresh;


};
EventDesktopResize* event_desktop_resize_new(int head_id);
void event_desktop_resize_free(EventDesktopResize* event_desktop_resize);
void event_desktop_resize_show(EventDesktopResize* event_desktop_resize);
void event_desktop_resize_json_serialise(EventDesktopResize* event_desktop_resize, char * buffer,int  buffer_size);
//------------------------------------------


//-------- event_connection_info ----------------------------------
// sent from the peer to the connection manager
typedef struct event_connection_info EventConnectionInfo;
struct event_connection_info
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int client_id; //the clients id
	freerdp_peer * client; //pointer to the client object
	int connection_type; //indicates if the client is requesting a multicast connection
	BOOL preemption_requested; //indicates if the client is requesting a preemptible connection
	UINT32 preemption_time;

};
EventConnectionInfo * event_connection_info_new(int client_id,BOOL multicast_requested);
void event_connection_info_free(EventConnectionInfo* event_res_change_complete);
void event_connection_info_show(EventConnectionInfo* event_res_change_complete);
void event_connection_info_json_serialise(EventConnectionInfo* event_res_change_complete, char * buffer,int  buffer_size);
//------------------------------------------




//-------- client_res_change_complete ----------------------------------
// sent from the peer to the connection manager
typedef struct event_res_change_complete EventResChangeComplete;
struct event_res_change_complete
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int client_id; //the clients id
	freerdp_peer * client; //pointer to the client object
	int connection_type; //indicates if the client is requesting a multicast connection
	BOOL preemption_requested; //indicates if the client is requesting a preemptible connection
	UINT32 preemption_time;
	UINT32 sync_loss;
	int head;
};
EventResChangeComplete* event_res_change_complete_new(int client_id,BOOL multicast_requested);
void event_res_change_complete_free(EventResChangeComplete* event_res_change_complete);
void event_res_change_complete_show(EventResChangeComplete* event_res_change_complete);
void event_res_change_complete_json_serialise(EventResChangeComplete* event_res_change_complete, char * buffer,int  buffer_size);
//------------------------------------------




//-------- client_ready Event ----------------------------------
// sent from the peer to the connection manager
typedef struct event_client_ready 	EventClientReady;
struct event_client_ready
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int client_id; //the clients id
	int connection_type; //indicates if the client is requesting a multicast connection
	BOOL preemption_requested; //indicates if the client is requesting a preemptible connection
	//DOMAIN_KEY
//	UINT32 domain_key;
	UINT8 domain_key[6];
	UINT32 session_id;
	char  loggedin_user[32];

};
//DOMAIN_KEY
EventClientReady* event_client_ready_new(int client_id,BOOL multicast_requested,BOOL preemption,UINT8 * domain_key,UINT32 session_id,char * loggedin_user);
void event_client_ready_free(EventClientReady* event_client_ready);
void event_client_ready_show(EventClientReady* event_client_ready);
void event_client_ready_json_serialise(EventClientReady* event_client_ready, char * buffer,int  buffer_size);
//------------------------------------------



//-------- server_peer_ready Event ----------------------------------
// sent from the peer to the connection manager
typedef struct event_server_peer_ready EventServerPeerReady;
struct event_server_peer_ready
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int server_peer_id; //the clients id
	freerdp_peer* client;


};
EventServerPeerReady* event_server_peer_ready_new(int server_peer_id);
void event_server_peer_ready_free(EventServerPeerReady* event_server_peer_ready);
void event_server_peer_ready_show(EventServerPeerReady* event_server_peer_ready);
void event_server_peer_ready_json_serialise(EventServerPeerReady* event_server_peer, char * buffer,int  buffer_size);
//------------------------------------------



//-------- marker Event ----------------------------------
// sent from anywhere
typedef struct event_marker EventMarker;
struct event_marker
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	char field1[255]; //general info field
	char field2[255]; //general info field
	char field3[255]; //general info field
	char field4[255]; //general info field
	char field5[255]; //general info field

};

EventMarker* event_marker_new(char * field1,char * field2,char * field3,char * field4,char * field5);
void event_marker_free(EventMarker* event_marker);
void event_marker_show(EventMarker* event_marker);
void event_marker_json_serialise(EventMarker* event_marker, char * buffer,int  buffer_size);
//------------------------------------------



//-------- fault Event ----------------------------------
// sent from anywhere
typedef struct event_error_info EventErrorInfo;
struct event_error_info
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int error_code;

};

EventErrorInfo * event_error_info_new(int error_code);
void event_error_info_free(EventErrorInfo* event_error_info);
void event_error_info_show(EventErrorInfo* event_error_info);
void event_error_info_json_serialise(EventErrorInfo * event_error_info, char * buffer,int  buffer_size);
//------------------------------------------



//-------- fault Event ----------------------------------
// sent from anywhere
typedef struct event_report_fault EventReportFault;
struct event_report_fault
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	FaultType fault_id; // fault ID
	char field2[255]; //general info field
	char field3[255]; //general info field
	char field4[255]; //general info field
	char field5[255]; //general info field

};

EventReportFault * event_report_fault_new(FaultType fault,char * field2,char * field3,char * field4,char * field5);
void event_report_fault_free(EventReportFault* event_report_fault);
void event_report_fault_show(EventReportFault* event_report_fault);
void event_report_fault_json_serialise(EventReportFault * event_report_fault, char * buffer,int  buffer_size);
//------------------------------------------


//-------- encode_irq Event ----------------------------------
// sent from anywhere
typedef struct event_encode_irq EventEncodeIRQ;

struct event_encode_irq
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	UINT32 previous_frame_time;
	char field1[255]; //general info field
	char field2[255]; //general info field
	char field3[255]; //general info field
	char field4[255]; //general info field

};

EventEncodeIRQ* event_encode_irq_new(char * field1,char * field2,char * field3,UINT32 previous_frame_time);
void event_encode_irq_free(EventEncodeIRQ* event_encode_irq);
void event_encode_irq_show(EventEncodeIRQ* event_encode_irq);
void event_encode_irq_json_serialise(EventEncodeIRQ* event_encode_irq, char * buffer,int  buffer_size);
//------------------------------------------



//-------- audio_irq Event ----------------------------------
// sent from anywhere
typedef struct event_audio_irq EventAudioIRQ;

struct event_audio_irq
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	char field1[255]; //general info field
	char field2[255]; //general info field
	char field3[255]; //general info field
	char field4[255]; //general info field

};

EventAudioIRQ* event_audio_irq_new(char * field1,char * field2,char * field3,char * field4);
void event_audio_irq_free(EventAudioIRQ* event_audio_irq);
void event_audio_irq_show(EventAudioIRQ* event_audio_irq);
void event_audio_irq_json_serialise(EventAudioIRQ* event_audio_irq, char * buffer,int  buffer_size);
//------------------------------------------


//-------- virtual_irq Event ----------------------------------
// sent from anywhere
typedef struct event_virtual_irq EventVirtualIRQ;

struct event_virtual_irq
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	char field1[255]; //general info field
	char field2[255]; //general info field
	char field3[255]; //general info field
	char field4[255]; //general info field

};

EventVirtualIRQ* event_virtual_irq_new(char * field1,char * field2,char * field3,char * field4);
void event_virtual_irq_free(EventVirtualIRQ* event_virtual_irq);
void event_virtual_irq_show(EventVirtualIRQ* event_audio_irq);
void event_virtual_irq_json_serialise(EventVirtualIRQ* event_virtual_irq, char * buffer,int  buffer_size);
//------------------------------------------

//-------- suspend_media Event ----------------------------------
// sent from the peer to the connection manager
typedef struct event_suspend_media EventSuspendMedia;
struct event_suspend_media
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
	//------ all events must start with these members
	int client_id; //the clients id


};
EventSuspendMedia* event_suspend_media_new(int client_id);
void event_suspend_media_free(EventSuspendMedia* event_suspend_media);
void event_suspend_media_show(EventSuspendMedia* event_suspend_media);
//------------------------------------------

#endif //__CORE_EVENT_H
