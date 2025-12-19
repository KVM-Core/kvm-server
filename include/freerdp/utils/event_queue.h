/**
 * FreeRDP: A Remote Desktop Protocol Client
 * X11 Server Event Handling
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

#ifndef __EVENT_QUEUE_H
#define __EVENT_QUEUE_H

typedef struct eq_event eqEvent;
typedef struct eq_event_queue eqEventQueue;
typedef enum fault_type
{
	MOUSE_WRITE_FAULT,
	KEYBOARD_WRITE_FAULT,
	FPGA_OPERATION_FAILURE,
	FPGA_RESET_FAILURE,
	BLI_THREAD_LOCKUP,
	FIF_EXCEPTION

}FaultType;
#include <pthread.h>
#include <freerdp/types.h>
#include <freerdp/utils/memory.h>




//the order of these is important, add new events to the end
enum eq_type
{
	EQ_EVENT_MARKER, //0
	EQ_EVENT_NO_OP, //1
	EQ_EVENT_CONTROL, //2
	EQ_EVENT_END, // 3 end the thread, process or loop associated with this queue
	EQ_EVENT_MOUSE, //4
	EQ_EVENT_KEYBOARD, //5
	EQ_EVENT_RESOLUTION_CHANGE, //6
	EQ_EVENT_SYNC_LOSS, //7
	EQ_EVENT_NEW_CONNECTION, //8
	EQ_EVENT_VIDEO_FRAME, //9 is this actually used????
	EQ_EVENT_DESKTOP_RESIZE, //10
	EQ_EVENT_SURFACE_COMMAND_AVAILABLE,  //11
	EQ_EVENT_PEER_TERMINATED, //12
	EQ_EVENT_CLIENT_SIDE_READY, //13
	EQ_EVENT_SURFACE_COMMAND_SENT,  //14
	EQ_EVENT_ENCODE_IRQ, //15
	EQ_EVENT_REPORT_FAULT, //16
	EQ_EVENT_KEYBOARD_OUTPUT_REPORT, //17
	EQ_EVENT_AUDIO_IRQ, //18
	EQ_EVENT_AUDIO_COMMAND_AVAILABLE,  //19
	EQ_EVENT_CONNECTION_STATUS,  //20
	EQ_EVENT_SERVER_PEER_READY, //21
	EQ_EVENT_CHANNEL_READY, //22
	EQ_EVENT_ENCODE_DONE,  //23
	EQ_EVENT_AUDIO_DONE, //24
	EQ_EVENT_SURFACE_COMMAND_COMPLETE, //25
	EQ_EVENT_FASTPATH_COMMAND_RECEIVED,  //26
	EQ_EVENT_SURFACE_COMMAND_PROCESSED, //27
	EQ_EVENT_VIRTUAL_IRQ, //28
	EQ_EVENT_VIRTUAL_DONE, //29
	EQ_EVENT_USB_COMMAND_AVAILABLE, //30
	EQ_EVENT_PEER_ACCESS_STATUS, //31
    EQ_EVENT_RECOVERY_REQUEST,  //32
    EQ_EVENT_RES_CHANGE_COMPLETE, //33
    EQ_EVENT_CLOUDIUM_MESSAGE, //34
	EQ_EVENT_INPUT, //35
	EQ_EVENT_ERROR_INFO, //36
	EQ_EVENT_CONNECTION_INFO, //37
};

struct eq_event
{
	int type;
	UINT32 send_time;
	UINT32 receive_time;
};

struct eq_event_queue
{
	int size;
	int count;
	int pipe_fd[2];
	eqEvent** events;
	pthread_mutex_t mutex;
	char name[255];
};

void eq_push(eqEventQueue* event_queue, eqEvent* event);
void eq_push_and_trace(eqEventQueue* event_queue, eqEvent* event, const char * caller);
eqEvent* eq_peek(eqEventQueue* event_queue);
eqEvent* eq_pop(eqEventQueue* event_queue);
int eq_queue_size(eqEventQueue* event_queue);
int eq_queue_length(eqEventQueue* event_queue);
int eq_get_queue_fd(eqEventQueue* event_queue);


#ifdef MEMORY_ALLOCATION_MONITOR
eqEventQueue* eq_queue_new(const char * function_name);
#else
eqEventQueue* eq_queue_new();
#endif

#ifdef MEMORY_ALLOCATION_MONITOR
void eq_queue_free(eqEventQueue* event_queue,const char * function_name);
#else
void eq_queue_free(eqEventQueue* event_queue);
#endif

eqEvent* eq_event_new(int type);
void eq_event_free(eqEvent*);

void eq_signal_set(int* fds);
void eq_signal_clear(int* fds);

int eq_is_signal_set(eqEventQueue* event_queue);

void eq_set_event(eqEventQueue* event_queue);

void eq_clear_event_queue(eqEventQueue* event_queue);
void eq_clear_events(eqEventQueue* event_queue);

void eq_purge(int event_type, eqEventQueue* event_queue);

void eq_show_event_type(int id);

void eq_set_name(eqEventQueue* event_queue,const char * name);

#endif /* __EVENT_QUEUE_H */
