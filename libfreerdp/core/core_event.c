
/*
 * John O'Sullivan
 * Copyright: Cloudium Systems 2014
 */

#include <freerdp/utils/sh_logger.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/event_queue.h>

#include <corrib_logger.h>
#include <textfields.h>

#include <freerdp/core_event.h>
#include "mcs.h"


static int cmd_counter = 0;
//END_EVENT
//-------------------------------------------------------------
//Signals to a queue dependent device that the processing function should end
//Constructor
EventEnd* event_end_new()
{
	EventEnd* event_end = xnew(EventEnd,__func__);

	if (event_end != NULL)
	{
		event_end->type = EQ_EVENT_END;
		event_end->send_time = 0;
		event_end->receive_time = 0;

	}

	return event_end;
}

//Destructor
void event_end_free(EventEnd* event_end)
{
	if(event_end)
	{
		xfree(event_end,__func__);
	}
}

//---------------------------------------------------

//KeyboardOutputReport Event
//----------------------------------------------------
//Constructor
EventKeyboard* event_keyboard_new(UINT16 code,UINT16 flags,int peer_id)
{
	EventKeyboard* event_keyboard = xnew(EventKeyboard,__func__);

	if (event_keyboard != NULL)
	{
		event_keyboard->type = EQ_EVENT_KEYBOARD;
		event_keyboard->send_time = 0;
		event_keyboard->receive_time = 0;
		event_keyboard->flags = flags;
		event_keyboard->code = code;
		event_keyboard->peer_id = peer_id;
	}

	return event_keyboard;
}

//Destructor
void event_keyboard_free(EventKeyboard* event_keyboard)
{
	if(event_keyboard)
	{
		xfree(event_keyboard,__func__);
	}
}

//to stdout
void event_keyboard_show(EventKeyboard* event_keyboard)
{
	if(event_keyboard)
	{
		printf("-------\n");
		printf("peer_id: %d\n",event_keyboard->peer_id);
		printf("code: %d\n",event_keyboard->code);
		printf("flags: %d\n",event_keyboard->flags);
		printf("received: %u\n",event_keyboard->receive_time);
		printf("sent: %u\n",event_keyboard->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_keyboard_json_serialise(EventKeyboard* event_keyboard, char * buffer,int  buffer_size)
{
	if(event_keyboard)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u','Field2': '%u','Field3': '%d'   }",
				EQ_EVENT_KEYBOARD,event_keyboard->send_time,event_keyboard->receive_time,event_keyboard->code,event_keyboard->flags,event_keyboard->peer_id);
	}
}


//---------------------------------------------------


//KeyboardOutputReport Event
//----------------------------------------------------
//Constructor
EventKeyboardOutputReport* event_keyboard_output_report_new(UINT8 mask)
{
	EventKeyboardOutputReport* event_keyboard_output_report = xnew(EventKeyboardOutputReport,__func__);

	if (event_keyboard_output_report != NULL)
	{
		event_keyboard_output_report->type = EQ_EVENT_KEYBOARD_OUTPUT_REPORT;
		event_keyboard_output_report->send_time = 0;
		event_keyboard_output_report->receive_time = 0;
		event_keyboard_output_report->mask = mask;

	}

	return event_keyboard_output_report;
}

//Destructor
void event_keyboard_output_report_free(EventKeyboardOutputReport* event_keyboard_output_report)
{
	if(event_keyboard_output_report)
	{
		xfree(event_keyboard_output_report,__func__);
	}
}

//to stdout
void event_keyboard_output_report_show(EventKeyboardOutputReport* event_keyboard_output_report)
{
	if(event_keyboard_output_report)
	{
		printf("-------\n");
		printf("mask: %d\n",event_keyboard_output_report->mask);
		printf("received: %u\n",event_keyboard_output_report->receive_time);
		printf("sent: %u\n",event_keyboard_output_report->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_keyboard_output_report_json_serialise(EventKeyboardOutputReport* event_keyboard_output_report, char * buffer,int  buffer_size)
{
	if(event_keyboard_output_report)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u','Field2': '','Field3': ' '   }",
				EQ_EVENT_KEYBOARD_OUTPUT_REPORT ,event_keyboard_output_report->send_time,event_keyboard_output_report->receive_time,event_keyboard_output_report->mask);
	}
}


//---------------------------------------------------
//---------------------------------------------------

//Input Event
//----------------------------------------------------
//Constructor
EventInput* event_input_new(int x,int y,UINT16 flags,UINT16 input_type)
{
	EventInput* event_input = xnew(EventInput,__func__);

	if (event_input != NULL)
	{
		event_input->type = EQ_EVENT_INPUT;
		event_input->send_time = 0;
		event_input->receive_time = 0;
		event_input->flags = flags;
		event_input->x = x;
		event_input->y = y;
		event_input->input_type = input_type;
		event_input->previous_frame_receive_time = 0;

	}

	return event_input;
}

//Destructor
void event_input_free(EventInput* event_input)
{
	if(event_input)
	{
		xfree(event_input,__func__);
	}
}

//to stdout
void event_input_show(EventInput* event_input)
{
	if(event_input)
	{
		printf("-------\n");
		printf("input_type: %d\n",event_input->input_type);
		printf("x: %u\n",event_input->x);
		printf("y: %u\n",event_input->y);
		printf("flags: %d\n",event_input->flags);
		printf("received: %u\n",event_input->receive_time);
		printf("sent: %u\n",event_input->send_time);
		printf("previous frame receive: %u\n",event_input->previous_frame_receive_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_input_json_serialise(EventInput* event_input, char * buffer,int  buffer_size)
{
	if(event_input)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u','Field2': '%u','Field3': %d, 'Field4': %d, 'Field5': %u  }",
				EQ_EVENT_INPUT,event_input->send_time,event_input->receive_time,event_input->x,event_input->y,event_input->flags,event_input->input_type,event_input->previous_frame_receive_time);
	}
}




//---------------------------------------------------
//---------------------------------------------------

//Mouse Event
//----------------------------------------------------
//Constructor
EventMouse* event_mouse_new(int x,int y,UINT16 flags,int peer_id)
{
	EventMouse* event_mouse = xnew(EventMouse,__func__);

	if (event_mouse != NULL)
	{
		event_mouse->type = EQ_EVENT_MOUSE;
		event_mouse->send_time = 0;
		event_mouse->receive_time = 0;
		event_mouse->flags = flags;
		event_mouse->x = x;
		event_mouse->y = y;
		event_mouse->peer_id = peer_id;
		event_mouse->previous_frame_receive_time = 0;

	}

	return event_mouse;
}

//Destructor
void event_mouse_free(EventMouse* event_mouse)
{
	if(event_mouse)
	{
		xfree(event_mouse,__func__);
	}
}

//to stdout
void event_mouse_show(EventMouse* event_mouse)
{
	if(event_mouse)
	{
		printf("-------\n");
		printf("peer_id: %d\n",event_mouse->peer_id);
		printf("x: %u\n",event_mouse->x);
		printf("y: %u\n",event_mouse->y);
		printf("flags: %d\n",event_mouse->flags);
		printf("received: %u\n",event_mouse->receive_time);
		printf("sent: %u\n",event_mouse->send_time);
		printf("previous frame receive: %u\n",event_mouse->previous_frame_receive_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_mouse_json_serialise(EventMouse* event_mouse, char * buffer,int  buffer_size)
{
	if(event_mouse)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u','Field2': '%u','Field3': %d, 'Field4': %d, 'Field5': %u  }",
				EQ_EVENT_MOUSE,event_mouse->send_time,event_mouse->receive_time,event_mouse->x,event_mouse->y,event_mouse->flags,event_mouse->peer_id,event_mouse->previous_frame_receive_time);
	}
}


//---------------------------------------------------

//resolution_change Event
//----------------------------------------------------
//Constructor
EventCloudiumMessage* event_cloudium_message_new(freerdp_peer* client1,UINT32 command,freerdp_peer* client2,char * username,UINT32 preemption_time)
{
	EventCloudiumMessage* event_cloudium_message = xnew(EventCloudiumMessage,__func__);

	if (event_cloudium_message != NULL)
	{
		event_cloudium_message->type = EQ_EVENT_CLOUDIUM_MESSAGE;
		event_cloudium_message->send_time = 0;
		event_cloudium_message->receive_time = 0;
		event_cloudium_message->client1 = client1;
		event_cloudium_message->client2 = client2;
		event_cloudium_message->command = command;
		if(username)
			strncpy(event_cloudium_message->username,username,254);
		else
			strncpy(event_cloudium_message->username,"unknown",254);
		event_cloudium_message->preemption_time = preemption_time;

	}

	return event_cloudium_message;
}

//Destructor
void event_cloudium_message_free(EventCloudiumMessage* event_cloudium_message)
{
	if(event_cloudium_message)
	{
		xfree(event_cloudium_message,__func__);
	}
}

//to stdout
void event_cloudium_message_show(EventCloudiumMessage* event_cloudium_message)
{
	if(event_cloudium_message)
	{
		printf("-------\n");
		printf("received: %u\n",event_cloudium_message->receive_time);
		printf("sent: %u\n",event_cloudium_message->send_time);

		printf("-------\n");
	}
}



//---------------------------------------------------

//resolution_change Event
//----------------------------------------------------
//Constructor
EventResolutionChange* event_resolution_change_new(int head_id, const videoData_t connectionResData)
{
    EventResolutionChange* event_resolution_change = xnew(EventResolutionChange,__func__);

    if (event_resolution_change != NULL)
    {
        event_resolution_change->type = EQ_EVENT_RESOLUTION_CHANGE;
        event_resolution_change->send_time = 0;
        event_resolution_change->receive_time = 0;
        event_resolution_change->head_id = head_id;
		event_resolution_change->connectionResData = connectionResData;
	}
    return event_resolution_change;
}

//Destructor
void event_resolution_change_free(EventResolutionChange* event_resolution_change)
{
	if(event_resolution_change)
	{
		xfree(event_resolution_change,__func__);
	}
}

//to stdout
void event_resolution_change_show(EventResolutionChange* event_resolution_change)
{
	if(event_resolution_change)
	{
		printf("-------\n");
		printf("head_id: %d\n",event_resolution_change->head_id);
		printf("received: %u\n",event_resolution_change->receive_time);
		printf("sent: %u\n",event_resolution_change->send_time);
		// printf("width: %u\n",event_resolution_change->connectionResData.ingress_resolution.width);
		// printf("height: %u\n",event_resolution_change->connectionResData.ingress_resolution.height);
		// printf("height: %u\n",event_resolution_change->connectionResData.ingress_resolution.height);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
// void event_resolution_change_json_serialise(EventResolutionChange* event_resolution_change, char * buffer,int  buffer_size)
// {
// 	if(event_resolution_change)
// 	{
// 		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%u','Field3': '%u', 'Field4': ' '   }",
// 				EQ_EVENT_RESOLUTION_CHANGE,event_resolution_change->send_time,event_resolution_change->receive_time,event_resolution_change->head_id,
// 				event_resolution_change->connectionResData.ingress_resolution.width, event_resolution_change->connectionResData.ingress_resolution.height);
// 	}
// }

//sync_loss Event
//----------------------------------------------------
//Constructor
EventSyncLoss* event_sync_loss_new(int head_id)
{
	EventSyncLoss* event_sync_loss = xnew(EventSyncLoss,__func__);

	if (event_sync_loss != NULL)
	{
		event_sync_loss->type = EQ_EVENT_SYNC_LOSS;
		event_sync_loss->send_time = 0;
		event_sync_loss->receive_time = 0;
		event_sync_loss->head_id = head_id;

	}

	return event_sync_loss;
}

//Destructor
void event_sync_loss_free(EventSyncLoss* event_sync_loss)
{
	if(event_sync_loss)
	{
		xfree(event_sync_loss,__func__);
	}
}

//to stdout
void event_sync_loss_show(EventSyncLoss* event_sync_loss)
{
	if(event_sync_loss)
	{
		printf("-------\n");
		printf("head_id: %d\n",event_sync_loss->head_id);
		printf("received: %u\n",event_sync_loss->receive_time);
		printf("sent: %u\n",event_sync_loss->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_sync_loss_json_serialise(EventSyncLoss* event_sync_loss, char * buffer,int  buffer_size)
{
	if(event_sync_loss)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': ' ','Field3': ' ', 'Field4': ' '   }",
				EQ_EVENT_SYNC_LOSS,event_sync_loss->send_time,event_sync_loss->receive_time,event_sync_loss->head_id);
	}
}

//desktop_resize Event
//----------------------------------------------------
//Constructor
EventDesktopResize* event_desktop_resize_new(int head_id)
{
	EventDesktopResize* event_desktop_resize = xnew(EventDesktopResize,__func__);

	if (event_desktop_resize != NULL)
	{
		event_desktop_resize->type = EQ_EVENT_DESKTOP_RESIZE;
		event_desktop_resize->send_time = 0;
		event_desktop_resize->receive_time = 0;
		event_desktop_resize->head_id = head_id;


	}

	return event_desktop_resize;
}

//Destructor
void event_desktop_resize_free(EventDesktopResize* event_desktop_resize)
{
	if(event_desktop_resize)
	{
		xfree(event_desktop_resize,__func__);
	}
}

//to stdout
void event_desktop_resize_show(EventDesktopResize* event_desktop_resize)
{
	if(event_desktop_resize)
	{
		printf("-------\n");
		printf("head_id: %d\n",event_desktop_resize->head_id);
		printf("received: %u\n",event_desktop_resize->receive_time);
		printf("sent: %u\n",event_desktop_resize->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_desktop_resize_json_serialise(EventDesktopResize* event_desktop_resize, char * buffer,int  buffer_size)
{
	if(event_desktop_resize)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': ' ','Field3': ' ', 'Field4': ' ', 'Field5': ' '   }",
				EQ_EVENT_DESKTOP_RESIZE, event_desktop_resize->send_time,
				event_desktop_resize->receive_time,
				event_desktop_resize->head_id);
	}
}
//-------------------------------------------------------




//client_ready Event
//----------------------------------------------------
//Constructor
//DOMAIN_KEY
EventClientReady* event_client_ready_new(int client_id,int connection_type,BOOL preemption,UINT8 * domain_key,UINT32 session_id,char * loggedin_user)
{
	int i;
	EventClientReady* event_client_ready = xnew(EventClientReady,__func__);

	if (event_client_ready != NULL)
	{
		event_client_ready->type = EQ_EVENT_CLIENT_SIDE_READY;
		event_client_ready->send_time = 0;
		event_client_ready->receive_time = 0;
		event_client_ready->client_id = client_id;
		event_client_ready->connection_type = connection_type;
		event_client_ready->preemption_requested = preemption;
		su_strlcpy(event_client_ready->loggedin_user,loggedin_user,USERNAME_LENGTH);
		for(i=0;i<6;i++)
			event_client_ready->domain_key[i] = domain_key[i];
		event_client_ready->session_id = session_id;
		//event_client_ready->multicast_requested = false; //dis-allow until we have multicast operational on Corrib
	}

	return event_client_ready;
}

//Destructor
void event_client_ready_free(EventClientReady* event_client_ready)
{
	if(event_client_ready)
	{
		xfree(event_client_ready,__func__);
	}
}

//to stdout
void event_client_ready_show(EventClientReady* event_client_ready)
{
	if(event_client_ready)
	{
		printf("-------\n");
		printf("client_id: %d\n",event_client_ready->client_id);
		printf("multicast_requested: %d\n",event_client_ready->connection_type);
		printf("received: %u\n",event_client_ready->receive_time);
		printf("sent: %u\n",event_client_ready->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_client_ready_json_serialise(EventClientReady* event_client_ready, char * buffer,int  buffer_size)
{
	if(event_client_ready)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%d', 'Field4': ' ', 'Field5': ' '   }",
				EQ_EVENT_CLIENT_SIDE_READY, event_client_ready->send_time,event_client_ready->receive_time,event_client_ready->client_id,
				event_client_ready->connection_type,
				event_client_ready->preemption_requested);
	}
}



//res_change_complete Event
//----------------------------------------------------
//Constructor
EventConnectionInfo* event_connection_info_new(int client_id, int connection_type)
{
	EventConnectionInfo* event_connection_info = xnew(EventConnectionInfo,__func__);

	if (event_connection_info != NULL)
	{
		event_connection_info->type = EQ_EVENT_CONNECTION_INFO;
		event_connection_info->send_time = 0;
		event_connection_info->receive_time = 0;
		event_connection_info->client_id = client_id;
		event_connection_info->connection_type = connection_type;
		//event_connection_info->multicast_requested = false; //dis-allow until we have multicast operational on Corrib
	}

	return event_connection_info;
}

//Destructor
void event_connection_info_free(EventConnectionInfo* event_connection_info)
{
	if(event_connection_info)
	{
		xfree(event_connection_info,__func__);
	}
}

//to stdout
void event_connection_info_show(EventConnectionInfo* event_connection_info)
{
	if(event_connection_info)
	{
		printf("-------\n");
		printf("client_id: %d\n",event_connection_info->client_id);
		printf("connection_type: %d\n",event_connection_info->connection_type);
		printf("received: %u\n",event_connection_info->receive_time);
		printf("sent: %u\n",event_connection_info->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_connection_info_json_serialise(EventConnectionInfo* event_connection_info, char * buffer,int  buffer_size)
{
	if(event_connection_info)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%d', 'Field4': ' ', 'Field5': ' '   }",
				EQ_EVENT_CONNECTION_INFO, event_connection_info->send_time,event_connection_info->receive_time,event_connection_info->client_id,
				event_connection_info->connection_type,
				event_connection_info->preemption_requested);
	}
}


//res_change_complete Event
//----------------------------------------------------
//Constructor
EventResChangeComplete* event_res_change_complete_new(int client_id, int connection_type)
{
	EventResChangeComplete* event_res_change_complete = xnew(EventResChangeComplete,__func__);

	if (event_res_change_complete != NULL)
	{
		event_res_change_complete->type = EQ_EVENT_RES_CHANGE_COMPLETE;
		event_res_change_complete->send_time = 0;
		event_res_change_complete->receive_time = 0;
		event_res_change_complete->client_id = client_id;
		event_res_change_complete->connection_type = connection_type;
		//event_res_change_complete->multicast_requested = false; //dis-allow until we have multicast operational on Corrib
	}

	return event_res_change_complete;
}

//Destructor
void event_res_change_complete_free(EventResChangeComplete* event_res_change_complete)
{
	if(event_res_change_complete)
	{
		xfree(event_res_change_complete,__func__);
	}
}

//to stdout
void event_res_change_complete_show(EventResChangeComplete* event_res_change_complete)
{
	if(event_res_change_complete)
	{
		printf("-------\n");
		printf("client_id: %d\n",event_res_change_complete->client_id);
		printf("connection_type: %d\n",event_res_change_complete->connection_type);
		printf("received: %u\n",event_res_change_complete->receive_time);
		printf("sent: %u\n",event_res_change_complete->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_res_change_complete_json_serialise(EventResChangeComplete* event_res_change_complete, char * buffer,int  buffer_size)
{
	if(event_res_change_complete)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%d', 'Field4': ' ', 'Field5': ' '   }",
				EQ_EVENT_CLIENT_SIDE_READY, event_res_change_complete->send_time,event_res_change_complete->receive_time,event_res_change_complete->client_id,
				event_res_change_complete->connection_type,
				event_res_change_complete->preemption_requested);
	}
}


//server_peer_ready Event
//----------------------------------------------------
//Constructor
EventServerPeerReady* event_server_peer_ready_new(int server_peer_id)
{
	EventServerPeerReady* event_server_peer_ready = xnew(EventServerPeerReady,__func__);

	if (event_server_peer_ready != NULL)
	{
		event_server_peer_ready->type = EQ_EVENT_SERVER_PEER_READY;
		event_server_peer_ready->send_time = 0;
		event_server_peer_ready->receive_time = 0;
		event_server_peer_ready->server_peer_id = server_peer_id;

	}

	return event_server_peer_ready;
}

//Destructor
void event_server_peer_ready_free(EventServerPeerReady* event_server_peer_ready)
{
	if(event_server_peer_ready)
	{
		xfree(event_server_peer_ready,__func__);
	}
}

//to stdout
void event_server_peer_ready_show(EventServerPeerReady* event_server_peer_ready)
{
	if(event_server_peer_ready)
	{
		printf("-------\n");
		printf("server_peer_id: %d\n",event_server_peer_ready->server_peer_id);
		printf("received: %u\n",event_server_peer_ready->receive_time);
		printf("sent: %u\n",event_server_peer_ready->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_server_peer_ready_json_serialise(EventServerPeerReady* event_server_peer, char * buffer,int  buffer_size)
{
	if(event_server_peer)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': ' ','Field3': ' ', 'Field4': ' ', 'Field5': ' '   }",
				EQ_EVENT_SERVER_PEER_READY, event_server_peer->send_time,event_server_peer->receive_time,event_server_peer->server_peer_id);
	}
}


//-------------------------------------------------------

//marker Event
//----------------------------------------------------
//Constructor
EventMarker* event_marker_new(char * field1,char * field2,char * field3,char * field4, char * field5)
{
	EventMarker* event_marker = xnew(EventMarker,__func__);

	if (event_marker != NULL)
	{
		event_marker->type = EQ_EVENT_MARKER;
		event_marker->send_time = 0;
		event_marker->receive_time = 0;
		strncpy(event_marker->field1,field1,255);
		strncpy(event_marker->field2,field2,255);
		strncpy(event_marker->field3,field3,255);
		strncpy(event_marker->field4,field4,255);
		strncpy(event_marker->field5,field5,255);

	}

	return event_marker;
}

//Destructor
void event_marker_free(EventMarker* event_marker)
{
	if(event_marker)
	{
		xfree(event_marker,__func__);
	}
}

//to stdout
void event_marker_show(EventMarker* event_marker)
{
	if(event_marker)
	{
		printf("-------\n");
		printf("received: %u\n",event_marker->receive_time);
		printf("sent: %u\n",event_marker->send_time);
		printf("field1: %s\n",event_marker->field1);
		printf("field2: %s\n",event_marker->field2);
		printf("field3: %s\n",event_marker->field3);
		printf("field4: %s\n",event_marker->field4);
		printf("field5: %s\n",event_marker->field5);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_marker_json_serialise(EventMarker* event_marker, char * buffer,int  buffer_size)
{
	if(event_marker)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%s','Field3': '%s', 'Field4': '%s', 'Field5': '%s'   }",
				EQ_EVENT_MARKER, event_marker->send_time,event_marker->receive_time,event_marker->field1,event_marker->field2,event_marker->field3,event_marker->field4,event_marker->field5);
	}
}
//-------------------------------------------------------


//-------------------------------------------------------

//fault Event
//----------------------------------------------------
//Constructor
EventReportFault* event_report_fault_new(FaultType fault,char * field2,char * field3,char * field4,char * field5)
{
	EventReportFault* event_report_fault = xnew(EventReportFault,__func__);

	if (event_report_fault != NULL)
	{
		event_report_fault->type = EQ_EVENT_REPORT_FAULT;
		event_report_fault->send_time = 0;
		event_report_fault->receive_time = 0;
		event_report_fault->fault_id = fault;
		strncpy(event_report_fault->field2,field2,255);
		strncpy(event_report_fault->field3,field3,255);
		strncpy(event_report_fault->field4,field4,255);
		strncpy(event_report_fault->field5,field5,255);

	}

	return event_report_fault;
}

//Destructor
void event_report_fault_free(EventReportFault* event_report_fault)
{
	if(event_report_fault)
	{	
		//do the field char arrays need to be freed? 
		xfree(event_report_fault,__func__);
	}
}

//to stdout
void event_report_fault_show(EventReportFault* event_report_fault)
{
	if(event_report_fault)
	{
		printf("-------\n");
		printf("received: %u\n",event_report_fault->receive_time);
		printf("sent: %u\n",event_report_fault->send_time);
		printf("Fault ID: %u\n",event_report_fault->fault_id);
		printf("field2: %s\n",event_report_fault->field2);
		printf("field3: %s\n",event_report_fault->field3);
		printf("field4: %s\n",event_report_fault->field4);
		printf("field5: %s\n",event_report_fault->field5);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_report_fault_json_serialise(EventReportFault* event_report_fault, char * buffer,int  buffer_size)
{
	if(event_report_fault)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u','Field2': '%s','Field3': '%s', 'Field4': '%s' , 'Field5': '%s', 'Field6': ' ' }",
				EQ_EVENT_REPORT_FAULT, event_report_fault->send_time,event_report_fault->receive_time,event_report_fault->fault_id,event_report_fault->field2,event_report_fault->field3,
				event_report_fault->field4,event_report_fault->field5);
	}
}
//-------------------------------------------------------




//-------------------------------------------------------

//fault Event
//----------------------------------------------------
//Constructor
EventErrorInfo* event_error_info_new(int error_code)
{
	EventErrorInfo* event_error_info = xnew(EventErrorInfo,__func__);

	if (event_error_info != NULL)
	{
		event_error_info->type = EQ_EVENT_ERROR_INFO;
		event_error_info->send_time = 0;
		event_error_info->receive_time = 0;
		event_error_info->error_code = error_code;


	}

	return event_error_info;
}

//Destructor
void event_error_info_free(EventErrorInfo* event_error_info)
{
	if(event_error_info)
	{
		xfree(event_error_info,__func__);
	}
}

//to stdout
void event_error_info_show(EventErrorInfo* event_error_info)
{
	if(event_error_info)
	{
		printf("-------\n");
		printf("received: %u\n",event_error_info->receive_time);
		printf("sent: %u\n",event_error_info->send_time);
		printf("Fault ID: %u\n",event_error_info->error_code);

		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_error_info_json_serialise(EventErrorInfo* event_error_info, char * buffer,int  buffer_size)
{
	if(event_error_info)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u' }",
				EQ_EVENT_REPORT_FAULT, event_error_info->send_time,event_error_info->receive_time,event_error_info->error_code);
	}
}
//-------------------------------------------------------







//-------------------------------------------------------

//encode_irq Event
//----------------------------------------------------
//Constructor
EventEncodeIRQ* event_encode_irq_new(char * field1,char * field2,char * field3,UINT32 previous_frame_time)
{
	EventEncodeIRQ* event_encode_irq = xnew(EventEncodeIRQ,__func__);

	if (event_encode_irq != NULL)
	{
		event_encode_irq->type = EQ_EVENT_ENCODE_IRQ;
		event_encode_irq->send_time = 0;
		event_encode_irq->receive_time = 0;
		event_encode_irq->previous_frame_time = previous_frame_time;
		strncpy(event_encode_irq->field1,field1,255);
		strncpy(event_encode_irq->field2,field2,255);
		strncpy(event_encode_irq->field3,field3,255);


	}

	return event_encode_irq;
}

//Destructor
void event_encode_irq_free(EventEncodeIRQ* event_encode_irq)
{
	if(event_encode_irq)
	{
		xfree(event_encode_irq,__func__);
	}
}

//to stdout
void event_encode_irq_show(EventEncodeIRQ* event_encode_irq)
{
	if(event_encode_irq)
	{
		printf("-------\n");
		printf("received: %u\n",event_encode_irq->receive_time);
		printf("sent: %u\n",event_encode_irq->send_time);
		printf("field1: %s\n",event_encode_irq->field1);
		printf("field2: %s\n",event_encode_irq->field2);
		printf("field3: %s\n",event_encode_irq->field3);
		printf("field4: %d\n",event_encode_irq->previous_frame_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_encode_irq_json_serialise(EventEncodeIRQ* event_encode_irq, char * buffer,int  buffer_size)
{
	if(event_encode_irq)
	{
		UINT32 delta = event_encode_irq->receive_time - event_encode_irq->previous_frame_time;
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%s','Field3': '%s', 'Field4': '%u'   }",
				EQ_EVENT_ENCODE_IRQ, event_encode_irq->send_time,event_encode_irq->receive_time,event_encode_irq->field1,event_encode_irq->field2,event_encode_irq->field3,delta);
		//corrib_syslog(LOG_DEBUG,"delta:%u %u %u\n",delta,event_encode_irq->receive_time,event_encode_irq->previous_frame_time);
	}
}
//-------------------------------------------------------





//-------------------------------------------------------

//audio_irq Event
//----------------------------------------------------
//Constructor
EventAudioIRQ* event_audio_irq_new(char * field1,char * field2,char * field3,char * field4)
{
	EventAudioIRQ* event_audio_irq = xnew(EventAudioIRQ,__func__);

	if (event_audio_irq != NULL)
	{
		event_audio_irq->type = EQ_EVENT_AUDIO_IRQ;
		event_audio_irq->send_time = 0;
		event_audio_irq->receive_time = 0;
		strncpy(event_audio_irq->field1,field1,255);
		strncpy(event_audio_irq->field2,field2,255);
		strncpy(event_audio_irq->field3,field3,255);
		strncpy(event_audio_irq->field4,field4,255);
	}

	return event_audio_irq;
}

//Destructor
void event_audio_irq_free(EventAudioIRQ* event_audio_irq)
{
	if(event_audio_irq)
	{
		xfree(event_audio_irq,__func__);
	}
}

//to stdout
void event_audio_irq_show(EventAudioIRQ* event_audio_irq)
{
	if(event_audio_irq)
	{
		printf("-------\n");
		printf("received: %u\n",event_audio_irq->receive_time);
		printf("sent: %u\n",event_audio_irq->send_time);
		printf("field1: %s\n",event_audio_irq->field1);
		printf("field2: %s\n",event_audio_irq->field2);
		printf("field3: %s\n",event_audio_irq->field3);
		printf("field4: %s\n",event_audio_irq->field4);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_audio_irq_json_serialise(EventAudioIRQ* event_audio_irq, char * buffer,int  buffer_size)
{
	if(event_audio_irq)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%s','Field3': '%s', 'Field4': '%s'   }",
				EQ_EVENT_AUDIO_IRQ, event_audio_irq->send_time,event_audio_irq->receive_time,event_audio_irq->field1,event_audio_irq->field2,event_audio_irq->field3,event_audio_irq->field4);
	}
}
//-------------------------------------------------------


//-------------------------------------------------------
// virtual_irq Event
//----------------------------------------------------
//Constructor
EventVirtualIRQ* event_virtual_irq_new(char * field1,char * field2,char * field3,char * field4)
{
	EventVirtualIRQ* event_virtual_irq = xnew(EventVirtualIRQ,__func__);

	if (event_virtual_irq != NULL)
	{
		event_virtual_irq->type = EQ_EVENT_VIRTUAL_IRQ;
		event_virtual_irq->send_time = 0;
		event_virtual_irq->receive_time = 0;
		strncpy(event_virtual_irq->field1,field1,255);
		strncpy(event_virtual_irq->field2,field2,255);
		strncpy(event_virtual_irq->field3,field3,255);
		strncpy(event_virtual_irq->field4,field4,255);
	}

	return event_virtual_irq;
}

//Destructor
void event_virtual_irq_free(EventVirtualIRQ* event_virtual_irq)
{
	if(event_virtual_irq)
	{
		xfree(event_virtual_irq,__func__);
	}
}

//to stdout
void event_virtual_irq_show(EventVirtualIRQ* event_virtual_irq)
{
	if(event_virtual_irq)
	{
		printf("-------\n");
		printf("received: %u\n",event_virtual_irq->receive_time);
		printf("sent: %u\n",event_virtual_irq->send_time);
		printf("field1: %s\n",event_virtual_irq->field1);
		printf("field2: %s\n",event_virtual_irq->field2);
		printf("field3: %s\n",event_virtual_irq->field3);
		printf("field4: %s\n",event_virtual_irq->field4);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_virtual_irq_json_serialise(EventVirtualIRQ* event_virtual_irq, char * buffer,int  buffer_size)
{
	if(event_virtual_irq)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%s','Field3': '%s', 'Field4': '%s'   }",
				EQ_EVENT_VIRTUAL_IRQ, event_virtual_irq->send_time,event_virtual_irq->receive_time,event_virtual_irq->field1,event_virtual_irq->field2,event_virtual_irq->field3,event_virtual_irq->field4);
	}
}
//-------------------------------------------------------



//suspend_media Event
//----------------------------------------------------
//Constructor
EventSuspendMedia* event_suspend_media_new(int client_id)
{
	EventSuspendMedia* event_suspend_media = xnew(EventSuspendMedia,__func__);

	if (event_suspend_media != NULL)
	{
		event_suspend_media->type = EQ_EVENT_CLIENT_SIDE_READY;
		event_suspend_media->send_time = 0;
		event_suspend_media->receive_time = 0;
		event_suspend_media->client_id = client_id;

	}

	return event_suspend_media;
}

//Destructor
void event_suspend_media_free(EventSuspendMedia * event_suspend_media)
{
	if(event_suspend_media)
	{
		xfree(event_suspend_media,__func__);
	}
}

//to stdout
void event_suspend_media_show(EventSuspendMedia * event_suspend_media)
{
	if(event_suspend_media)
	{
		printf("-------\n");
		printf("client_id: %d\n",event_suspend_media->client_id);
		printf("received: %u\n",event_suspend_media->receive_time);
		printf("sent: %u\n",event_suspend_media->send_time);
		printf("-------\n");
	}
}

//-------------------------------------------------------


//New Connection Event
//----------------------------------------------------
//Constructor
EventConnectionStatus* event_connection_status_new(int peer_sockfd,const char * hostname)
{
	EventConnectionStatus* event_connection_status = xnew(EventConnectionStatus,__func__);

	if (event_connection_status != NULL)
	{
		event_connection_status->type = EQ_EVENT_CONNECTION_STATUS;
		event_connection_status->send_time = 0;
		event_connection_status->receive_time = 0;
		event_connection_status->peer_sockfd = peer_sockfd;
		strncpy(event_connection_status->hostname,hostname,50);

	}

	return event_connection_status;
}

//Destructor
void event_connection_status_free(EventConnectionStatus* event_connection_status)
{
	if(event_connection_status)
	{
		xfree(event_connection_status,__func__);
	}
}

//to stdout
void event_connection_status_show(EventConnectionStatus* event_connection_status)
{
	if(event_connection_status)
	{
		printf("-------\n");
		printf("peer socket fd: %d\n",event_connection_status->peer_sockfd);
		printf("hostname: %s\n",event_connection_status->hostname);
		printf("received: %u\n",event_connection_status->receive_time);
		printf("sent: %u\n",event_connection_status->send_time);

		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_connection_status_json_serialise(EventConnectionStatus* event_connection_status, char * buffer,int  buffer_size)
{
	if(event_connection_status)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%d','Field3': ' ', 'Field4': ' '   }",
				EQ_EVENT_CONNECTION_STATUS, event_connection_status->send_time,event_connection_status->receive_time,event_connection_status->hostname,event_connection_status->peer_sockfd);
	}
}




//New Connection Event
//----------------------------------------------------
//Constructor
EventNewConnection* event_new_connection_new(int peer_sockfd,const char * hostname)
{
	EventNewConnection* event_new_connection = xnew(EventNewConnection,__func__);

	if (event_new_connection != NULL)
	{
		event_new_connection->type = EQ_EVENT_NEW_CONNECTION;
		event_new_connection->send_time = 0;
		event_new_connection->receive_time = 0;
		event_new_connection->peer_sockfd = peer_sockfd;
		strncpy(event_new_connection->hostname,hostname,50);

	}

	return event_new_connection;
}

//Destructor
void event_new_connection_free(EventNewConnection* event_new_connection)
{
	if(event_new_connection)
	{
		xfree(event_new_connection,__func__);
	}
}

//to stdout
void event_new_connection_show(EventNewConnection* event_new_connection)
{
	if(event_new_connection)
	{
		printf("-------\n");
		printf("peer socket fd: %d\n",event_new_connection->peer_sockfd);
		printf("hostname: %s\n",event_new_connection->hostname);
		printf("received: %u\n",event_new_connection->receive_time);
		printf("sent: %u\n",event_new_connection->send_time);

		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_new_connection_json_serialise(EventNewConnection* event_new_connection, char * buffer,int  buffer_size)
{
	if(event_new_connection)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%s','Field2': '%d','Field3': ' ', 'Field4': ' '   }",
				EQ_EVENT_NEW_CONNECTION, event_new_connection->send_time,event_new_connection->receive_time,event_new_connection->hostname,event_new_connection->peer_sockfd);
	}
}

//Surface Command Sent
//----------------------------------------------------
//Constructor
// EventSurfaceCommandSent* event_surface_command_sent_new(int head_id,FRAME_TYPES frame_type,int frame_number,void * cmd)
// {
// 	EventSurfaceCommandSent* event_surface_command_sent = xnew(EventSurfaceCommandSent,__func__);
// 	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_surface_command_sent);
// 	if (event_surface_command_sent != NULL)
// 	{
// 		event_surface_command_sent->type = EQ_EVENT_SURFACE_COMMAND_SENT;
// 		event_surface_command_sent->send_time = 0;
// 		event_surface_command_sent->receive_time = 0;
// 		event_surface_command_sent->head_id = head_id;
// 		event_surface_command_sent->frame_type = frame_type;
// 		event_surface_command_sent->frame_number = frame_number;
// 		event_surface_command_sent->cmd = cmd;
// 		event_surface_command_sent->command_available_time = 0;
// 	}

// 	return event_surface_command_sent;
// }

//Destructor
void event_surface_command_sent_free(EventSurfaceCommandSent* event_surface_command_sent)
{
	if(event_surface_command_sent)
	{
		xfree(event_surface_command_sent,__func__);
	}
}

// //to stdout
// void event_surface_command_sent_show(EventSurfaceCommandSent* event_surface_command_sent)
// {
// 	if(event_surface_command_sent)
// 	{
// 		printf("-------\n");
// 		printf("head ID: %d\n",event_surface_command_sent->head_id);
// 		printf("sent: %u\n",event_surface_command_sent->send_time);
// 		printf("received: %u\n",event_surface_command_sent->receive_time);
// 		printf("frame type: %u\n",event_surface_command_sent->frame_type);
// 		printf("frame number: %u\n",event_surface_command_sent->frame_number);
// 		printf("-------\n");
// 	}
// }


/*
 * The calling function must manage the memory
 */
void event_surface_command_sent_json_serialise(EventSurfaceCommandSent* event_surface_command_sent, char * buffer,int  buffer_size)
{
	if(event_surface_command_sent)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%d', 'Field4': ' '   }",
				EQ_EVENT_SURFACE_COMMAND_SENT, event_surface_command_sent->send_time,event_surface_command_sent->receive_time,event_surface_command_sent->head_id,event_surface_command_sent->frame_number,event_surface_command_sent->command_available_time);
	}
}


//Peer Access Status
//----------------------------------------------------
//Constructor
EventAccessStatus* event_peer_access_status_new(int peer_id,ACCESS_STATUS status)
{
	EventAccessStatus* event_access_status = xnew(EventAccessStatus,__func__);
	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_access_status);
	if (event_access_status != NULL)
	{
		event_access_status->peer_id = peer_id;
		event_access_status->type = EQ_EVENT_PEER_ACCESS_STATUS;
		event_access_status->send_time = 0;
		event_access_status->receive_time = 0;
		event_access_status->access_status = status;

	}

	return event_access_status;
}






//Destructor
void event_peer_access_status_free(EventAccessStatus* event_access_status)
{
	if(event_access_status)
	{
		xfree(event_access_status,__func__);
	}
}

//to stdout
void event_access_status_show(EventAccessStatus* event_access_status)
{
	if(event_access_status)
	{
		printf("-------\n");

		printf("sent: %u\n",event_access_status->send_time);
		printf("received: %u\n",event_access_status->receive_time);

		printf("-------\n");
	}
}


/*
 * The calling function must manage the memory
 */
void event_peer_access_status_json_serialise(EventAccessStatus* event_access_status, char * buffer,int  buffer_size)
{
	if(event_access_status)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': ' ','Field2': ' ','Field3': ' ', 'Field4': ' ','Field5': ' ','Field6': ' ','Field7': ' ' }",
				EQ_EVENT_PEER_ACCESS_STATUS, event_access_status->send_time,event_access_status->receive_time);
	}
}






//Peer Access Status
//----------------------------------------------------
//Constructor
EventRecoveryRequest* event_peer_recovery_request_new(int peer_id,int head_id)
{
	EventRecoveryRequest* event_recovery_request = xnew(EventRecoveryRequest,__func__);
	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_recovery_request);
	if (event_recovery_request != NULL)
	{
		event_recovery_request->head_id = head_id;
		event_recovery_request->type = EQ_EVENT_RECOVERY_REQUEST;
		event_recovery_request->send_time = 0;
		event_recovery_request->receive_time = 0;


	}

	return event_recovery_request;
}






//Destructor
void event_peer_recovery_request_free(EventRecoveryRequest* event_recovery_request)
{
	if(event_recovery_request)
	{
		xfree(event_recovery_request,__func__);
	}
}

//to stdout
void event_recovery_request_show(EventRecoveryRequest* event_recovery_request)
{
	if(event_recovery_request)
	{
		printf("-------\n");

		printf("sent: %u\n",event_recovery_request->send_time);
		printf("received: %u\n",event_recovery_request->receive_time);

		printf("-------\n");
	}
}


/*
 * The calling function must manage the memory
 */
void event_peer_recovery_status_json_serialise(EventRecoveryRequest* event_recovery_request, char * buffer,int  buffer_size)
{
	if(event_recovery_request)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': ' ','Field2': ' ','Field3': ' ', 'Field4': ' ','Field5': ' ','Field6': ' ','Field7': ' ' }",
				EQ_EVENT_RECOVERY_REQUEST, event_recovery_request->send_time,event_recovery_request->receive_time);
	}
}







//Surface Command Complete
//----------------------------------------------------
//Constructor
EventSurfaceCommandComplete* event_surface_command_complete_new(int head_id,int frame_number,void * cmd)
{
	EventSurfaceCommandComplete* event_surface_command_complete = xnew(EventSurfaceCommandComplete,__func__);
	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_surface_command_complete);
	if (event_surface_command_complete != NULL)
	{
		event_surface_command_complete->type = EQ_EVENT_SURFACE_COMMAND_SENT;
		event_surface_command_complete->send_time = 0;
		event_surface_command_complete->receive_time = 0;
		event_surface_command_complete->head_id = head_id;
		event_surface_command_complete->frame_number = frame_number;
		event_surface_command_complete->cmd = cmd;
		event_surface_command_complete->command_available_time = 0;
		event_surface_command_complete->command_sent_time = 0;
	}

	return event_surface_command_complete;
}


EventSurfaceCommandComplete* event_surface_command_complete_clone_from_available_command(EventSurfaceCommandAvailable *  event_surface_command_available)
{
	EventSurfaceCommandComplete* event_surface_command_complete = xnew(EventSurfaceCommandComplete,__func__);
	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_surface_command_complete);
	if (event_surface_command_complete != NULL)
	{
		event_surface_command_complete->type = EQ_EVENT_SURFACE_COMMAND_COMPLETE;
		event_surface_command_complete->send_time = 0;
		event_surface_command_complete->receive_time = 0;
		event_surface_command_complete->head_id = event_surface_command_available->head_id;
		event_surface_command_complete->frame_number = event_surface_command_available->frame_number;
		event_surface_command_complete->cmd = event_surface_command_available->cmd;
		event_surface_command_complete->command_sent_time = event_surface_command_available->send_time;
	}
	return event_surface_command_complete;
}

EventSurfaceCommandComplete* event_surface_command_complete_clone_from_sent_command(EventSurfaceCommandSent *  event_surface_command_sent)
{
	EventSurfaceCommandComplete* event_surface_command_complete = xnew(EventSurfaceCommandComplete,__func__);
	//corrib_syslog(LOG_DEBUG,"%s: allocating @ %p\n",__func__,event_surface_command_complete);
	if (event_surface_command_complete != NULL)
	{
		event_surface_command_complete->type = EQ_EVENT_SURFACE_COMMAND_COMPLETE;
		event_surface_command_complete->send_time = 0;
		event_surface_command_complete->receive_time = 0;
		event_surface_command_complete->head_id = event_surface_command_sent->head_id;
		event_surface_command_complete->frame_number = event_surface_command_sent->frame_number;
		event_surface_command_complete->cmd = event_surface_command_sent->cmd;
		event_surface_command_complete->command_sent_time = event_surface_command_sent->send_time;
	}
	return event_surface_command_complete;
}


//Destructor
void event_surface_command_complete_free(EventSurfaceCommandComplete* event_surface_command_complete)
{
	if(event_surface_command_complete)
	{
		xfree(event_surface_command_complete,__func__);
	}
}

//to stdout
void event_surface_command_complete_show(EventSurfaceCommandComplete* event_surface_command_complete)
{
	if(event_surface_command_complete)
	{
		printf("-------\n");
		printf("head ID: %d\n",event_surface_command_complete->head_id);
		printf("sent: %u\n",event_surface_command_complete->send_time);
		printf("received: %u\n",event_surface_command_complete->receive_time);
		printf("frame number: %u\n",event_surface_command_complete->frame_number);
		printf("-------\n");
	}
}


/*
 * The calling function must manage the memory
 */
void event_surface_command_complete_json_serialise(EventSurfaceCommandComplete* event_surface_command_complete, char * buffer,int  buffer_size)
{
	if(event_surface_command_complete)
	{
		UINT32 encode_time = event_surface_command_complete->encode_done_time - event_surface_command_complete->previous_frame_time;
		UINT32 packetize_time = event_surface_command_complete->command_available_time - event_surface_command_complete->encode_done_time;
		UINT32 encrypt_time =  event_surface_command_complete->receive_time - event_surface_command_complete->command_sent_time;
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%d', 'Field4': '%d','Field5': '%d','Field6': '%d','Field7': '%d' }",
				EQ_EVENT_SURFACE_COMMAND_COMPLETE, event_surface_command_complete->send_time,event_surface_command_complete->receive_time,
				event_surface_command_complete->head_id,event_surface_command_complete->frame_number,
				encode_time,packetize_time,encrypt_time,event_surface_command_complete->frame_size,event_surface_command_complete->dropped_frames);
	}
}

//Audio Command Available
//----------------------------------------------------
//Constructor
EventAudioCommandAvailable* event_audio_command_available_new(void * cmd)
{
	EventAudioCommandAvailable* event_audio_command_available = xnew(EventAudioCommandAvailable,__func__);

	if (event_audio_command_available != NULL)
	{
		event_audio_command_available->type = EQ_EVENT_AUDIO_COMMAND_AVAILABLE;
		event_audio_command_available->send_time = 0;
		event_audio_command_available->receive_time = 0;
		event_audio_command_available->cmd = cmd;
		event_audio_command_available->previous_frame_receive_time = 0;
	}

	return event_audio_command_available;
}

//Destructor
void event_audio_command_available_free(EventAudioCommandAvailable* event_audio_command_available)
{
	if(event_audio_command_available)
	{
		if(event_audio_command_available->cmd)
		{
			xfree(event_audio_command_available->cmd, __func__);
		}
		xfree(event_audio_command_available,__func__);
	}
}

//to stdout
void event_audio_command_available_show(EventAudioCommandAvailable* event_audio_command_available)
{
	if(event_audio_command_available)
	{
		printf("-------\n");
		printf("cmd: %p\n",event_audio_command_available->cmd);
		printf("size: %u\n",event_audio_command_available->size);
		printf("sent: %u\n",event_audio_command_available->send_time);
		printf("received: %u\n",event_audio_command_available->receive_time);
		printf("previous_frame_receive_time: %u\n",event_audio_command_available->previous_frame_receive_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_audio_command_available_json_serialise(EventAudioCommandAvailable* event_audio_command_available, char * buffer,int  buffer_size)
{
	if(event_audio_command_available)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': ' ','Field2': '%d','Field3': '%u' }",
				EQ_EVENT_AUDIO_COMMAND_AVAILABLE,
				event_audio_command_available->send_time,
				event_audio_command_available->receive_time,
				event_audio_command_available->previous_frame_receive_time,
				0);
	}
}

//Audio Channel Ready
//----------------------------------------------------
//Constructor
EventChannelReady* event_channel_ready_new(rdpMcsChannel* channel)
{
	EventChannelReady* event_channel_ready = xnew(EventChannelReady,__func__);

	if (event_channel_ready != NULL)
	{
		event_channel_ready->type = EQ_EVENT_CHANNEL_READY;
		event_channel_ready->send_time = 0;
		event_channel_ready->receive_time = 0;
		// event_channel_ready->channel = channel;
	}

	return event_channel_ready;
}

//Destructor
void event_channel_ready_free(EventChannelReady* event_channel_ready)
{
	if(event_channel_ready)
	{
		xfree(event_channel_ready,__func__);
	}
}

//to stdout
void event_channel_ready_show(EventChannelReady* event_channel_ready)
{
	if(event_channel_ready)
	{
		printf("-------\n");
		// printf("channel ID: %d\n",event_channel_ready->channel->ChannelId);
		printf("sent: %u\n",event_channel_ready->send_time);
		printf("received: %u\n",event_channel_ready->receive_time);
		// printf("channel name: %s\n",event_channel_ready->channel->Name);
		printf("-------\n");
	}
}


/*
 * The calling function must manage the memory
 */
// void event_channel_ready_json_serialise(EventChannelReady* event_channel_ready, char * buffer,int  buffer_size)
// {
// 	if(event_channel_ready)
// 	{
// 		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%s'  }",
// 				EQ_EVENT_CHANNEL_READY, event_channel_ready->send_time,event_channel_ready->receive_time,event_channel_ready->channel->ChannelId, event_channel_ready->channel->Name);
// 	}
// }


// RDPEUSB Command Available
//----------------------------------------------------
//Constructor
// EventUsbCommandAvailable* event_usb_command_available_new(sequenceData* sequence_data, int device_id, void * cmd)
// {
// 	EventUsbCommandAvailable* event_usb_command_available = xnew(EventUsbCommandAvailable,__func__);

// 	if (event_usb_command_available != NULL)
// 	{
// 		event_usb_command_available->type = EQ_EVENT_USB_COMMAND_AVAILABLE;
// 		event_usb_command_available->send_time = 0;
// 		event_usb_command_available->receive_time = 0;
// 		event_usb_command_available->sequence_data = sequence_data;
// 		event_usb_command_available->device_id = device_id;
// 		event_usb_command_available->cmd = cmd;
// 	}

// 	return event_usb_command_available;
// }

//Destructor
// void event_usb_command_available_free(EventUsbCommandAvailable* event_usb_command_available)
// {
// 	if(event_usb_command_available)
// 	{
// 		if (event_usb_command_available->cmd) {
// 			usb_command_free(event_usb_command_available->cmd);
// 			xfree(event_usb_command_available->sequence_data, __func__);
// 		}
// 		else if (event_usb_command_available->sequence_data) {
// 			xfree(event_usb_command_available->sequence_data, __func__);
// 		}
// 		xfree(event_usb_command_available,__func__);
// 	}
// }

//to stdout
void event_usb_command_available_show(EventUsbCommandAvailable* event_usb_command_available)
{
	if(event_usb_command_available)
	{
		printf("-------\n");
		// printf("seq data: %p\n",event_usb_command_available->sequence_data);
		printf("device id: %d\n",event_usb_command_available->device_id);
		printf("cmd: %p\n",event_usb_command_available->cmd);
		printf("size: %u\n",event_usb_command_available->size);
		printf("sent: %u\n",event_usb_command_available->send_time);
		printf("received: %u\n",event_usb_command_available->receive_time);
		printf("-------\n");
	}
}


/*
 * The calling function must manage the memory
 */
void event_usb_command_available_json_serialise(EventUsbCommandAvailable* event_usb_command_available, char * buffer,int  buffer_size)
{
	// if(event_usb_command_available)
	// {
	// 	snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'SequenceData': '%p','DeviceId': '%d' }",
	// 			EQ_EVENT_USB_COMMAND_AVAILABLE, event_usb_command_available->send_time,event_usb_command_available->receive_time,event_usb_command_available->sequence_data,event_usb_command_available->device_id);
	// }
}



//Surface Command Available
//----------------------------------------------------
//Constructor
// EventSurfaceCommandAvailable* event_surface_command_available_new(int head_id,void * cmd,FRAME_TYPES frame_type,int frame_number,int number_of_tiles,int dropped_frames,UINT32 frame_size)
// {
// 	char func_name[255];
// 	sprintf(func_name,"%s:%d",__func__,cmd_counter);
// 	EventSurfaceCommandAvailable* event_surface_command_available = xnew(EventSurfaceCommandAvailable,func_name);

// 	if (event_surface_command_available != NULL)
// 	{
// 		event_surface_command_available->type = EQ_EVENT_SURFACE_COMMAND_AVAILABLE;
// 		event_surface_command_available->send_time = 0;
// 		event_surface_command_available->receive_time = 0;
// 		event_surface_command_available->head_id = head_id;
// 		event_surface_command_available->cmd = cmd;
// 		event_surface_command_available->frame_type = frame_type;
// 		event_surface_command_available->frame_number = frame_number;
// 		event_surface_command_available->number_of_tiles = number_of_tiles;
// 		event_surface_command_available->previous_frame_receive_time = 0;
// 		event_surface_command_available->dropped_frames = dropped_frames;
// 		event_surface_command_available->is_duplicate = false;
// 		event_surface_command_available->frame_size = frame_size;
// 		event_surface_command_available->command_id = cmd_counter++;

// 	}

// 	return event_surface_command_available;
// }

//Copy Constructor
EventSurfaceCommandAvailable* event_surface_command_available_clone(EventSurfaceCommandAvailable* event)
{
	// EventSurfaceCommandAvailable* event_surface_command_available = xnew(EventSurfaceCommandAvailable,__func__);

	// if (event_surface_command_available != NULL)
	// {
	// 	if(event != NULL)
	// 	{
	// 		event_surface_command_available->type = EQ_EVENT_SURFACE_COMMAND_AVAILABLE;
	// 		event_surface_command_available->send_time = event->send_time;
	// 		event_surface_command_available->receive_time = event->receive_time;
	// 		event_surface_command_available->head_id = event->head_id;
	// 		event_surface_command_available->cmd = event->cmd;
	// 		event_surface_command_available->frame_type = event->frame_type;
	// 		event_surface_command_available->frame_number = event->frame_number;
	// 		event_surface_command_available->number_of_tiles = event->number_of_tiles;
	// 		event_surface_command_available->previous_frame_receive_time = event->previous_frame_receive_time;
	// 		event_surface_command_available->dropped_frames = event->dropped_frames;
	// 		event_surface_command_available->is_duplicate = true;
	// 		event_surface_command_available->encode_done_timestamp = event->encode_done_timestamp;
	// 		event_surface_command_available->frame_size = event->frame_size;
	// 		event_surface_command_available->command_id = event->command_id;
	// 	}
	// 	else
	// 		corrib_syslog(LOG_ERR,"%s: cannot clone a null event\n",__func__);
	// }

	// return event_surface_command_available;
}

//Destructor
void event_surface_command_available_free(EventSurfaceCommandAvailable* event_surface_command_available)
{
	if(event_surface_command_available)
	{
		xfree(event_surface_command_available,__func__);
	}
}

//to stdout
// void event_surface_command_available_show(EventSurfaceCommandAvailable* event_surface_command_available)
// {
// 	if(event_surface_command_available)
// 	{
// 		printf("-------\n");
// 		printf("head ID: %d\n",event_surface_command_available->head_id);
// 		printf("cmd: %p\n",event_surface_command_available->cmd);
// 		printf("size: %u\n",event_surface_command_available->size);
// 		printf("sent: %u\n",event_surface_command_available->send_time);
// 		printf("received: %u\n",event_surface_command_available->receive_time);
// 		printf("frame type: %u\n",event_surface_command_available->frame_type);
// 		printf("frame number: %u\n",event_surface_command_available->frame_number);
// 		printf("number_of_tiles: %u\n",event_surface_command_available->number_of_tiles);
// 		printf("previous_frame_receive_time: %u\n",event_surface_command_available->previous_frame_receive_time);
// 		printf("dropped_frames: %u\n",event_surface_command_available->dropped_frames);
// 		printf("Is duplicate: %d\n",event_surface_command_available->is_duplicate);
// 		printf("-------\n");
// 	}
// }

/*
 * The calling function must manage the memory
 */
void event_surface_command_available_json_serialise(EventSurfaceCommandAvailable* event_surface_command_available, char * buffer,int  buffer_size)
{
	if(event_surface_command_available)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%u', 'Field4': '%u'  , 'Field5': '%u' }",
				//EQ_EVENT_SURFACE_COMMAND_AVAILABLE, event_surface_command_available->send_time,event_surface_command_available->receive_time,event_surface_command_available->head_id,event_surface_command_available->frame_number,event_surface_command_available->previous_frame_receive_time,event_surface_command_available->number_of_tiles,event_surface_command_available->dropped_frames);
				EQ_EVENT_SURFACE_COMMAND_AVAILABLE, event_surface_command_available->send_time,event_surface_command_available->receive_time,event_surface_command_available->head_id,event_surface_command_available->frame_number,event_surface_command_available->previous_frame_receive_time,event_surface_command_available->number_of_tiles,event_surface_command_available->frames_in_flight);
	}
}





//Encode Done
//----------------------------------------------------
//Constructor
EventEncodeDone* event_encode_done_new(int head_id, FRAME_TYPES frame_type, int frame_number, int starting_tile, int number_of_tiles)
{
	EventEncodeDone* event_encode_done = xnew(EventEncodeDone,__func__);

	if (event_encode_done != NULL)
	{
		event_encode_done->type = EQ_EVENT_ENCODE_DONE;
		event_encode_done->send_time = 0;
		event_encode_done->receive_time = 0;
		event_encode_done->starting_tile = starting_tile;
		event_encode_done->head_id = head_id;
		event_encode_done->frame_type = frame_type;
		event_encode_done->frame_number = frame_number;
		event_encode_done->number_of_tiles = number_of_tiles;
		event_encode_done->previous_frame_receive_time = 0;

	}

	return event_encode_done;
}

//Destructor
void event_encode_done_free(EventEncodeDone* event_encode_done)
{
	if(event_encode_done)
	{
		xfree(event_encode_done,__func__);
	}
}
//Remove a video event from the queue and replace it with a no-op
void event_encode_done_video_purge( eqEventQueue* event_queue, int head)
{
	int i = 0;
	if((event_queue != NULL) )
	{
		for(i=0;i< event_queue->count;i++)
		{
			if(event_queue->events[i] != NULL)
			{
				if((event_queue->events[i]->type == EQ_EVENT_ENCODE_DONE))
				{
					EventEncodeDone* event_decode_done = (EventEncodeDone *) event_queue->events[i];
					if(event_decode_done->head_id == head)
						event_queue->events[i]->type = EQ_EVENT_NO_OP;
				}
			}
		}
		//corrib_syslog(LOG_INFO,"%s:Purged %d events of type ",__func__,i);
		//eq_show_event_type(event_type);
	}
}
//to stdout
void event_encode_done_show(EventEncodeDone* event_encode_done)
{
	// if(event_encode_done)
	// {
	// 	printf("-------\n");
	// 	printf("head ID: %d\n",event_encode_done->head_id);
	// 	printf("size: %u\n",event_encode_done->size);
	// 	printf("sent: %u\n",event_encode_done->send_time);
	// 	printf("received: %u\n",event_encode_done->receive_time);
	// 	printf("frame type: %u\n",event_encode_done->frame_type);
	// 	printf("frame number: %u\n",event_encode_done->frame_number);
	// 	printf("statring_tile: %u\n",event_encode_done->starting_tile);
	// 	printf("number_of_tiles: %u\n",event_encode_done->number_of_tiles);
	// 	printf("previous_frame_receive_time: %u\n",event_encode_done->previous_frame_receive_time);
	// 	printf("-------\n");
	// }
}

/*
 * The calling function must manage the memory
 */
void event_encode_done_json_serialise(EventEncodeDone* event_encode_done, char * buffer,int  buffer_size)
{
	if(event_encode_done)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': '%d','Field3': '%u', 'Field4': '%u'  , 'Field5': '%d' }",
				EQ_EVENT_ENCODE_DONE, event_encode_done->send_time,event_encode_done->receive_time,event_encode_done->head_id,event_encode_done->frame_number,event_encode_done->previous_frame_receive_time,event_encode_done->number_of_tiles,event_encode_done->starting_tile);
	}
}




//Audio Done
//----------------------------------------------------
//Constructor
EventAudioDone* event_audio_done_new(void)
{
	EventAudioDone* event_audio_done = xnew(EventAudioDone,__func__);

	if (event_audio_done != NULL)
	{
		event_audio_done->type = EQ_EVENT_AUDIO_DONE;
		event_audio_done->send_time = 0;
		event_audio_done->receive_time = 0;
	}

	return event_audio_done;
}

//Destructor
void event_audio_done_free(EventAudioDone* event_audio_done)
{
	if(event_audio_done)
	{
		xfree(event_audio_done,__func__);
	}
}

//to stdout
void event_audio_done_show(EventAudioDone* event_audio_done)
{
	if(event_audio_done)
	{
		printf("-------\n");
		printf("sent: %u\n",event_audio_done->send_time);
		printf("received: %u\n",event_audio_done->receive_time);
		printf("previous_frame_receive_time: %u\n",event_audio_done->previous_frame_receive_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_audio_done_json_serialise(EventAudioDone* event_audio_done, char * buffer,int  buffer_size)
{
	if(event_audio_done)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%u'}",
				EQ_EVENT_AUDIO_DONE, event_audio_done->send_time,event_audio_done->receive_time,event_audio_done->previous_frame_receive_time);
	}
}


//Virtual Done
//----------------------------------------------------
//Constructor
// EventVirtualDone* event_virtual_done_new(virtualDevice *device)
// {
// 	EventVirtualDone* event_virtual_done = xnew(EventVirtualDone,__func__);

// 	if (event_virtual_done != NULL)
// 	{
// 		event_virtual_done->type = EQ_EVENT_VIRTUAL_DONE;
// 		event_virtual_done->send_time = 0;
// 		event_virtual_done->receive_time = 0;
// 		event_virtual_done->device = device;
// 	}

// 	return event_virtual_done;
// }

//Destructor
void event_virtual_done_free(EventVirtualDone* event_virtual_done)
{
	if(event_virtual_done)
	{
		xfree(event_virtual_done,__func__);
	}
}

//to stdout
void event_virtual_done_show(EventVirtualDone* event_virtual_done)
{
	// if(event_virtual_done)
	// {
	// 	printf("-------\n");
	// 	printf("device ID: %d\n",event_virtual_done->device->device_id);
	// 	printf("sent: %u\n",event_virtual_done->send_time);
	// 	printf("received: %u\n",event_virtual_done->receive_time);
	// 	printf("previous_frame_receive_time: %u\n",event_virtual_done->previous_frame_receive_time);
	// 	printf("-------\n");
	// }
}

/*
 * The calling function must manage the memory
 */
void event_virtual_done_json_serialise(EventVirtualDone* event_virtual_done, char * buffer,int  buffer_size)
{
	// if(event_virtual_done)
	// {
	// 	snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'DeviceId': '%d', 'Field2': '%u'}",
	// 			EQ_EVENT_VIRTUAL_DONE, event_virtual_done->send_time,event_virtual_done->receive_time,event_virtual_done->device->device_id, event_virtual_done->previous_frame_receive_time);
	// }
}



//peer_terminated Event
//----------------------------------------------------
//Constructor
EventPeerTerminated* event_peer_terminated_new(int peer_id,int peer_cid)
{
	EventPeerTerminated* event_peer_terminated = xnew(EventPeerTerminated,__func__);

	if (event_peer_terminated != NULL)
	{
		event_peer_terminated->type = EQ_EVENT_PEER_TERMINATED;
		event_peer_terminated->send_time = 0;
		event_peer_terminated->receive_time = 0;
		event_peer_terminated->peer_id = peer_id;
		event_peer_terminated->peer_cid = peer_cid;
	}

	return event_peer_terminated;
}

//Destructor
void event_peer_terminated_free(EventPeerTerminated* event_peer_terminated)
{
	if(event_peer_terminated)
	{
		xfree(event_peer_terminated,__func__);
	}
}

//to stdout
void event_peer_terminated_show(EventPeerTerminated* event_peer_terminated)
{
	if(event_peer_terminated)
	{
		printf("-------\n");
		printf("peer_id: %d\n",event_peer_terminated->peer_id);
		printf("peer_cid: %d\n",event_peer_terminated->peer_cid);
		printf("received: %u\n",event_peer_terminated->receive_time);
		printf("sent: %u\n",event_peer_terminated->send_time);
		printf("-------\n");
	}
}

/*
 * The calling function must manage the memory
 */
void event_peer_terminated_json_serialise(EventPeerTerminated* event_peer_terminated, char * buffer,int  buffer_size)
{
	if(event_peer_terminated)
	{
		snprintf(buffer,buffer_size,"{'EventType': %d,'StartTime': %u,'EndTime': %u,'Field1': '%d','Field2': ' ','Field3': ' ', 'Field4': ' '   }",
				EQ_EVENT_PEER_TERMINATED,event_peer_terminated->send_time,event_peer_terminated->receive_time,event_peer_terminated->peer_id);
	}
}
