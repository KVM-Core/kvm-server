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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
//#include <X11/Xlib.h>
//#define VERBOSE=1

#include <freerdp/utils/memory.h>
#include <freerdp/utils/sh_logger.h>

#include <freerdp/utils/event_queue.h>
#include <corrib_logger.h>

int eq_get_queue_fd(eqEventQueue* event_queue)
{
	if(event_queue)
		return event_queue->pipe_fd[0];
	else
		return -1;
}

void eq_signal_set(int* fds)
{
	int length;
	length = write(fds[1], "sig", 4);
	if (length != 4)
		corrib_syslog(LOG_ERR,"sh_signal_set: error %d \n",length);
}

void eq_signal_clear(int* fds)
{
	int length;
	length = read(fds[0], &length, 4);
	if (length != 4)
		corrib_syslog(LOG_ERR,"sh_signal_clear: error\n");
}

int eq_is_signal_set(eqEventQueue* event_queue)
{
	fd_set rfds;
	int num_set;
	struct timeval time;

	FD_ZERO(&rfds);
	FD_SET(event_queue->pipe_fd[0], &rfds);
	memset(&time, 0, sizeof(time));
	num_set = select(event_queue->pipe_fd[0] + 1, &rfds, 0, 0, &time);

	return (num_set == 1);
}

//set the signal associated with this queue
void eq_set_event(eqEventQueue* event_queue)
{
	int length;

	length = write(event_queue->pipe_fd[1], "sig", 4);

	if (length != 4)
		corrib_syslog(LOG_ERR,"eq_set_event: error in %s\n",event_queue->name);
}

void eq_clear_events(eqEventQueue* event_queue)
{
	if(event_queue != NULL)
	{
               #ifdef VERBOSE
               corrib_syslog(LOG_DEBUG, "%s: Clearing queue %s",__func__,event_queue->name);
               #endif
		while(event_queue->count)
		{
			eqEvent* event = eq_pop(event_queue);
			if(event)
				xfree(event,__func__);
		}
	}
}



//reset the signal associated with the queue
void eq_clear_event(eqEventQueue* event_queue)
{
	int length;
        
        #ifdef VERBOSE
        corrib_syslog(LOG_DEBUG, "%s: B Read in queue %s",__func__,event_queue->name);
        #endif
	length = read(event_queue->pipe_fd[0], &length, 4);

	if (length != 4)
		corrib_syslog(LOG_ERR,"sh_clear_event: error in %s\n",event_queue->name);
}

void eq_push_and_trace(eqEventQueue* event_queue, eqEvent* event, const char * caller)
{
	if(event_queue)
	{
        unsigned int ret=0;

        corrib_syslog(LOG_DEBUG, "%s: %s: %s: BL",__func__,caller,event_queue->name);
  
        ret = pthread_mutex_lock(&(event_queue->mutex));
        if(ret)
        {
            switch(ret)
            {
                case EDEADLK:
                {
                    corrib_syslog(LOG_ERR, "%s: A deadlock condition was detected in %s\n",__func__,event_queue->name);
                    break;
                }

                default:
                {
                    
                    corrib_syslog(LOG_DEBUG, "%s: pthread_mutex_lock encountered an unknown error in %s",__func__,event_queue->name);
                  
                }
            }
        }

		if (event_queue->count >= event_queue->size)
		{
			event_queue->size *= 2;
			event_queue->events = (eqEvent**) xrealloc((void*) event_queue->events, sizeof(eqEvent*) * event_queue->size,__func__);
            corrib_syslog(LOG_DEBUG, "%s: %s:Resize,event_queue->size=%u",__func__,caller,event_queue->size);
		}

		event_queue->events[(event_queue->count)++] = event;

		pthread_mutex_unlock(&(event_queue->mutex));

        corrib_syslog(LOG_DEBUG, "%s: %s: %s AL",__func__,caller,event_queue->name);

		eq_set_event(event_queue); //set the queue signal
	}
	else
		corrib_syslog(LOG_ERR,"%s:writing to queue %s that is not initialised\n",__func__,event_queue->name);
}

void eq_push(eqEventQueue* event_queue, eqEvent* event)
{
	if(event_queue)
	{
        unsigned int ret=0;
        #ifdef VERBOSE
        corrib_syslog(LOG_DEBUG, "%s: %s BL",__func__,event_queue->name);
        #endif
        ret = pthread_mutex_lock(&(event_queue->mutex));
        if(ret)
        {
            switch(ret)
            {
                case EDEADLK:
                {
                    corrib_syslog(LOG_ERR, "%s: A deadlock condition was detected in %s\n",__func__,event_queue->name);
                    break;
                }

                default:
                {
                    #ifdef VERBOSE
                        corrib_syslog(LOG_DEBUG, "%s: pthread_mutex_lock acquire failed in %s",__func__,event_queue->name);
                    #endif
                }
            }
        }

		if (event_queue->count >= event_queue->size)
		{
			event_queue->size *= 2;
			event_queue->events = (eqEvent**) xrealloc((void*) event_queue->events, sizeof(eqEvent*) * event_queue->size,__func__);
		}

		event_queue->events[(event_queue->count)++] = event;

		pthread_mutex_unlock(&(event_queue->mutex));
                #ifdef VERBOSE
                corrib_syslog(LOG_DEBUG, "%s: %s AUL",__func__,event_queue->name);
                #endif
		eq_set_event(event_queue); //set the queue signal
	}
	else
		corrib_syslog(LOG_ERR,"%s:writing to queue %s that is not initialised\n",__func__,event_queue->name);
}

int eq_queue_size(eqEventQueue* event_queue)
{
	return event_queue->size;
}

int eq_queue_length(eqEventQueue* event_queue)
{
	return event_queue->count;
}
/* Can only peek at the next event */
eqEvent* eq_peek(eqEventQueue* event_queue)
{
	eqEvent* event = NULL;
	if(event_queue)
	{
        unsigned int ret=0;
        #ifdef VERBOSE
        corrib_syslog(LOG_DEBUG, "%s: %s BL",__func__,event_queue->name);
        #endif
        ret = pthread_mutex_lock(&(event_queue->mutex));
        if(ret)
        {
            switch(ret)
            {
                case EDEADLK:
                {
                    corrib_syslog(LOG_ERR, "%s: A deadlock condition was detected in %s\n",__func__,event_queue->name);
                    break;
                }

                default:
                {
                    #ifdef VERBOSE
                        corrib_syslog(LOG_DEBUG, "%s: pthread_mutex_lock failed during acquire in %s",__func__,event_queue->name);
                    #endif
                }
            }
        }
		if (event_queue->count < 1)
			event = NULL;
		else
			event = event_queue->events[0];

		pthread_mutex_unlock(&(event_queue->mutex));
                #ifdef VERBOSE
                corrib_syslog(LOG_DEBUG, "%s: %s AUL",__func__,event_queue->name);
                #endif

	}
	else
		corrib_syslog(LOG_ERR,"%s:peeking a queue %s that is not initialised\n",__func__,event_queue->name);
	return event;
}

//Remove a particular type of event from the queue and replace it with a no-op
void eq_purge(int event_type, eqEventQueue* event_queue)
{

	if(event_queue != NULL)
	{
        int i = 0;
		for(i=0;i< event_queue->count;i++)
		{
			if(event_queue->events[i]->type == event_type)
				event_queue->events[i]->type = EQ_EVENT_NO_OP;
		}
		//corrib_syslog(LOG_INFO,"%s:Purged %d events of type ",__func__,i);
		//eq_show_event_type(event_type);
	}
}





eqEvent* eq_pop(eqEventQueue* event_queue)
{
	eqEvent* event = NULL;
	if(event_queue)
	{


        unsigned int ret=0;
        #ifdef VERBOSE
        corrib_syslog(LOG_DEBUG, "%s: %s: BL",__func__,event_queue->name);
        #endif
        ret = pthread_mutex_lock(&(event_queue->mutex));
        if(ret)
        {
            switch(ret)
            {
                case EDEADLK:
                {
                    corrib_syslog(LOG_ERR, "%s: A deadlock condition was detected in %s\n",__func__,event_queue->name);
                    break;
                }

                default:
                {
                    #ifdef VERBOSE
                        corrib_syslog(LOG_DEBUG, "%s: Failed to acquire pthread_mutex_lock in %s",__func__,event_queue->name);
                    #endif
                }
            }
        }

	if (event_queue->count < 1)
        {
                #ifdef VERBOSE
                corrib_syslog(LOG_DEBUG, "%s: %s: queue count is zero, unlock and return",__func__,event_queue->name);
                #endif
                pthread_mutex_unlock(&(event_queue->mutex)); //we cannot return without unlocking the mutex
		return NULL;
        }
		/* remove event signal */
		eq_clear_event(event_queue);

		event = event_queue->events[0];
		(event_queue->count)--;

		memmove(&event_queue->events[0], &event_queue->events[1], event_queue->count * sizeof(void*));

		pthread_mutex_unlock(&(event_queue->mutex));
                #ifdef VERBOSE
                corrib_syslog(LOG_DEBUG, "%s: %s AUL",__func__,event_queue->name);
                #endif

	}
	else
		corrib_syslog(LOG_ERR,"%s:writing to a queue %s that is not initialised\n",__func__,event_queue->name);
	return event;
}








eqEvent* eq_event_new(int type)
{
	eqEvent* event = xnew(eqEvent,__func__);
	event->type = type;
	return event;
}

void eq_event_free(eqEvent* event)
{
	if(event)
	{
		xfree(event,__func__);
	}
}

void eq_set_name(eqEventQueue* event_queue,const char * name)
{
	strncpy(event_queue->name,name,255);
}

#ifdef MEMORY_ALLOCATION_MONITOR
eqEventQueue* eq_queue_new(const char * function_name)
#else
eqEventQueue* eq_queue_new()

#endif
{
    pthread_mutexattr_t mutex_attr;
    int mutex_type, ret=0;
#ifdef MEMORY_ALLOCATION_MONITOR
	eqEventQueue* event_queue = xnew(eqEventQueue,function_name);
#else
	eqEventQueue* event_queue = xnew(eqEventQueue,__func__);
#endif


	if (event_queue != NULL)
	{
		event_queue->pipe_fd[0] = -1;
		event_queue->pipe_fd[1] = -1;

		event_queue->size = 16;
		event_queue->count = 0;
#ifdef MEMORY_ALLOCATION_MONITOR
		event_queue->events = (eqEvent**) xzalloc(sizeof(eqEvent*) * event_queue->size,function_name);
#else
		event_queue->events = (eqEvent**) xzalloc(sizeof(eqEvent*) * event_queue->size,__func__);

#endif




    pthread_mutexattr_init(&mutex_attr);
    ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    if(ret)
        corrib_syslog(LOG_ERR ,"%s: Failed to set mutex type",__func__);
		pthread_mutex_init(&(event_queue->mutex), &mutex_attr);
		if (pipe(event_queue->pipe_fd) < 0)
			corrib_syslog(LOG_ERR,"eq_queue_new: pipe failed\n");


	}

	return event_queue;
}

#ifdef MEMORY_ALLOCATION_MONITOR
void eq_queue_free(eqEventQueue* event_queue,const char * function_name)
#else
void eq_queue_free(eqEventQueue* event_queue)
#endif
{
	if (event_queue->pipe_fd[0] != -1)
	{
		close(event_queue->pipe_fd[0]);
		event_queue->pipe_fd[0] = -1;
	}

	if (event_queue->pipe_fd[1] != -1)
	{
		close(event_queue->pipe_fd[1]);
		event_queue->pipe_fd[1] = -1;
	}

	xfree(event_queue->events,__func__);
	xfree(event_queue,__func__);

	pthread_mutex_destroy(&(event_queue->mutex));
}

//FIXME, this needs to reflect all events
void eq_show_event_type(int id)
{
	corrib_syslog(LOG_DEBUG ,"Event Type:");
	switch(id)
	{
	case 	EQ_EVENT_MARKER:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_MARKER");
		break;
	case 	EQ_EVENT_NO_OP:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_NO_OP");
		break;
	case 	EQ_EVENT_CONTROL:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_CONTROL");
		break;
	case 	EQ_EVENT_END:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_END,");
		break;
	case 	EQ_EVENT_MOUSE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_MOUSE");
		break;
	case	EQ_EVENT_KEYBOARD:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_KEYBOARD");
		break;
	case 	EQ_EVENT_RESOLUTION_CHANGE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_RESOLUTION_CHANGE");
		break;
	case 	EQ_EVENT_SYNC_LOSS:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_SYNC_LOSS");
		break;
	case 	EQ_EVENT_NEW_CONNECTION:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_NEW_CONNECTION");
		break;
	case 	EQ_EVENT_VIDEO_FRAME:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_VIDEO_FRAME(%d)",id);
		break;
	case 	EQ_EVENT_ENCODE_DONE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_DECODE_DONE(%d)",id);
		break;
	case 	EQ_EVENT_DESKTOP_RESIZE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_DESKTOP_RESIZE");
		break;
	case 	EQ_EVENT_SURFACE_COMMAND_AVAILABLE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_SURFACE_COMMAND_AVAILABLE");
		break;
	case 	EQ_EVENT_SURFACE_COMMAND_COMPLETE:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_SURFACE_COMMAND_COMPLETE");
		break;
	case 	EQ_EVENT_CONNECTION_INFO:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_CONNECTION_INFO");
		break;
	case 	EQ_EVENT_SURFACE_COMMAND_SENT:
		corrib_syslog(LOG_DEBUG ,"EQ_EVENT_SURFACE_COMMAND_SENT");
		break;


	default:
		corrib_syslog(LOG_ERR ,"%s: unknown event %d\n",__func__,id);

	}

}
