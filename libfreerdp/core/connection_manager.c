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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>
#include <sys/select.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

#include <freerdp/utils/memory.h>
#include <freerdp/connection_manager.h>

cmContext* connection_manager_new(char * trace_target)
{
	cmContext* cm_context = xnew(cmContext, __func__);
	if (!cm_context)
		return NULL;
#if 0
	su_get_tx_mouse_keyboard_timeout(&(cm_context->mouse_keyboard_timeout));
	cm_context->hm_context = hw_manager_new(); //we create the hardware manager and store a reference
	cm_context->mouse_keyboard_available = true;
	cm_context->controlling_peer_id = -1;
	cm_context->cm_compression_mode = UNKNOWN_COMPRESSION;
#if defined(_EMERALD4K)
	cm_context->server_technology_type = EMERALD_4K_SERVER;
#else
	cm_context->server_technology_type = EMERALD_2K_SERVER;
#endif
	cm_context->preemption = false;
	cm_context->client_id_counter = 1; //0 is reserved for unallocated
	//create  event queues for the hardware manager
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->hm_cm_queue = eq_queue_new(__func__);
#else
	cm_context->hm_cm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->hm_cm_queue,"hm_cm_queue");
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->cm_hm_queue = eq_queue_new(__func__);
#else
	cm_context->cm_hm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->cm_hm_queue,"cm_hm_queue");
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->peer_cm_queue = eq_queue_new(__func__);
#else
	cm_context->peer_cm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->peer_cm_queue,"peer_cm_queue");
	cm_context->cm_operating_mode = UNKNOWN_CONNECTION_MODE;
	//cm_context->cm_operating_mode = PREEMPTIVE;
	cm_context->multicast_peer_client = NULL;
	memset(cm_context->audio_channels, CM_AV_CHANNEL_UNUSED, MAX_SHARED_CONNECTIONS);
	memset(cm_context->video_channels, CM_AV_CHANNEL_UNUSED, MAX_SHARED_CONNECTIONS);
	cm_context->resolution_change_needed[HEAD_1] = false;
	cm_context->resolution_change_needed[HEAD_2] = false;
	cm_context->statistics_counter = 60;

	//corrib_syslog(LOG_DEBUG,"%s: cm_context->peer_cm_queue = %p\n",__func__,cm_context->peer_cm_queue);
    LIST_INIT(&(cm_context->peer_list_head));                       /* Initialize the peer list. */
    TAILQ_INIT(&(cm_context->peer_wait_queue_head));   //for peers waiting to join
    pthread_mutex_init(&(cm_context->mutex), NULL);
	cm_context->performance_analysis = false;
	hw_manager_set_queues(cm_context->hm_context,cm_context->hm_cm_queue,cm_context->cm_hm_queue); //set the hw manager's queues
	connection_manager_init_slave_cid_pool(cm_context);
#endif
	return cm_context;
}

void connection_manager_enable_performance_analysis(cmContext * cm_context)
{
	cm_context->performance_analysis = true;
	// if(cm_context->hm_context)
	// 	hw_manager_enable_performance_analysis(cm_context->hm_context);
}

void connection_manager_set_queues(cmContext * cm_context,eqEventQueue* listener_queue)
{

	cm_context->listener_queue = listener_queue;
}

// static void * connection_manager_main_loop(void * arg)
// {
// }

void connection_manager_run(cmContext * cm_context)
{

	// statistcs_send_json_control_object("flush_active_connections");

	// if(su_toe_init(TRANSMITTER))
	// {
	// 	corrib_syslog(LOG_ERR,"CM: Failed to initialise TOE config, terminating\n");
	// 	exit(0);
	// }
	// if(su_avae_init())
	// {
	// 	corrib_syslog(LOG_ERR,"CM: Failed to initialise AVAE config, terminating\n");
	// 	exit(0);
	// }

	// cm_context->main_thread = connection_manager_create_thread(connection_manager_main_loop, cm_context);
	// cm_context->video_municast_running = 0;
	// cm_context->audio_municast_running = 0;
	// cm_context->video_multicast_running = 0;
	// cm_context->audio_multicast_running = 0;
	// cm_context->connecting_client = NULL;
	// if(cm_context->main_thread == -1)
	// {
	// 	corrib_syslog(LOG_ERR,"CM:Failed to create thread for connection_manager,terminating\n");
	// 	exit(0);
	// }
	// // if(freerdp_check_file_exists("/usr/local/FPGA_RESET_TEST"))
	// // {
	// // 	corrib_syslog(LOG_DEBUG,"Entering FPGA reset test mode\n");
	// // 	hw_manager_test_fpga_reset(cm_context->hm_context);
	// // }
	// // else
	// // 	hw_manager_run(cm_context->hm_context); //start the hardware manager

}





#if 0

#include "connection_manager_state_machine.h"
#include <freerdp/bitops.h>
#include <freerdp/freerdp.h>
#include <freerdp/mpeer.h>
#include <freerdp/core_event.h>
#include <freerdp/utils/sh_logger.h>
#include <freerdp/utils/memory.h>
#include "rdp.h"
#include <freerdp/utils/queue.h>
#include <corrib_logger.h>
//#include "statistics_json.h"
#include <restapi.h>

#include "event_processor.h"
#include <errorcodes.h>
#include <textfields.h>
#include <system_avae.h>
#include <system_toe.h>

//#define VERBOSE_DEBUGGING



#include "client.h"
#include "bb_connection.h"

#define MAX_FDS 32

//#define CONNECTION_PROFILING

//#define USBR_SEQUENCE_TRACING

// #define CM_PRINT_FUNC() _cm_print_func_name(__func__)
#define CM_PRINT_FUNC()

enum cm_av_channel_state {
	CM_AV_CHANNEL_UNUSED = 0,
	CM_AV_CHANNEL_ALLOCATED,
	CM_AV_CHANNEL_LISTENING,
	CM_AV_CHANNEL_ACTIVE,
};

struct sockaddr_nl src_addr;

static const char *compression_type_name[] = { COMPRESSION_ENUMS(AS_STR) };
static const char *connection_type_name[] = { CONNECTION_MODE_ENUMS(AS_STR) };

static pthread_t connection_manager_create_thread( void* func, void* arg);
static void * connection_manager_main_loop(void * arg);
static void connection_manager_handle_new_client_request(cmContext * cm_context,int client_socket_fd, const char * hostname);
static void connection_manager_set_connecting_client(cmContext * cm_context, freerdp_peer* client, const char * caller);
static void connection_manager_unset_connecting_client(cmContext * cm_context, freerdp_peer* client, const char * caller);
static boolean connection_manager_getActive_extDsktp(cmContext * cm_context, freerdp_peer * client);
static peerNode * connection_manager_find_client_node(cmContext * cm_context,freerdp_peer* client);
static void connection_manager_show_peer_list_entry(peerNode * node);
static void connection_manager_handle_fault_report(cmContext * cm);
static void connection_manager_show_peer_list(cmContext * cm);
static void connection_manager_terminate_peer(freerdp_peer * client);


static freerdp_peer * connection_manager_get_peer_for_fd(cmContext * cm_context,int fd);
static void connection_manager_handle_peer_termination(cmContext * cm_context, int client_sock_fd);

static void connection_manager_handle_resolution_change(cmContext * cm_context, int head, const videoData_t connectionResData);
static boolean connection_manager_handle_resolution_change_for_client(freerdp_peer * client, int head, const videoData_t connectionResData);
static void connection_manager_handle_sync_loss(cmContext * cm_context,int head);
static void connection_manager_handle_sync_loss_for_client(freerdp_peer * client, int head);

void connection_manager_handle_keyboard_output_report(cmContext * cm_context,uint8 mask);
static void connection_manager_handle_audio_command(cmContext * cm_context,EventAudioCommandAvailable * audio_command_available_event);
static void connection_manager_handle_usb_command(cmContext * cm_context,EventUsbCommandAvailable * usb_command_available_event);
static void connection_manager_handle_client_ready(cmContext * cm_context,int client_socket_fd,int connection_type,boolean preemption_requested,uint8 * domain_key,uint32 session_id,char * loggedin_user);
static bool connection_manager_handle_res_change_complete(cmContext * cm_context, int client_socket_fd,
							  int head);
static void connection_manager_handle_connection_info(cmContext * cm_context,EventConnectionInfo * client_connection_info);
static boolean connection_manager_handle_server_peer_ready(cmContext * cm_context,freerdp_peer * client);
static void connection_manager_arbitrate_keyboard_mouse_control(cmContext * cm_context,int peer_id, int exclusive_mode_request);
static void connection_manager_init_slave_cid_pool(cmContext * cm_context);
static void connection_manager_process_wait_queue(cmContext * cm_context);
unsigned int connection_manager_get_slave_cid(cmContext * cm_context,av_mode mode,freerdp_peer * client);
static void connection_manager_show_slave_cid_pool(cmContext * cm_context);
static void connection_manager_show_connection_details(cmContext * cm_context,freerdp_peer * client, const char * func,const char * comment);
static void connection_manager_check_missed_res_events(cmContext *cm_context, freerdp_peer *client, int head);
static void connection_manager_check_sync_loss(cmContext *cm_context, freerdp_peer *client, int head);
static const char * cm_get_peer_video_state_string(const rdp_peer_video_state video_state);
static const char * cm_get_peer_connection_state_string(const rdp_peer_connection_state connection_state);
/*
 * The connection manager is always running, it is created by the listner and iterares its list of peers
 * The listner can add new connections
 */


void _cm_print_func_name(const char *func_name) {
     corrib_syslog(LOG_DEBUG, "[CONNECTION_MANAGER]: %s();\n", func_name);
}

/*
 * Constructor
 */
cmContext *  connection_manager_new(char * trace_target)
{

	cmContext * cm_context =  xnew(cmContext,__func__);
	su_get_tx_mouse_keyboard_timeout(&(cm_context->mouse_keyboard_timeout));
	cm_context->hm_context = hw_manager_new(); //we create the hardware manager and store a reference
	cm_context->mouse_keyboard_available = true;
	cm_context->controlling_peer_id = -1;
	cm_context->cm_compression_mode = UNKNOWN_COMPRESSION;
#if defined(_EMERALD4K)
	cm_context->server_technology_type = EMERALD_4K_SERVER;
#else
	cm_context->server_technology_type = EMERALD_2K_SERVER;
#endif
	cm_context->preemption = false;
	cm_context->client_id_counter = 1; //0 is reserved for unallocated
	//create  event queues for the hardware manager
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->hm_cm_queue = eq_queue_new(__func__);
#else
	cm_context->hm_cm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->hm_cm_queue,"hm_cm_queue");
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->cm_hm_queue = eq_queue_new(__func__);
#else
	cm_context->cm_hm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->cm_hm_queue,"cm_hm_queue");
#ifdef MEMORY_ALLOCATION_MONITOR
	cm_context->peer_cm_queue = eq_queue_new(__func__);
#else
	cm_context->peer_cm_queue = eq_queue_new();
#endif
        eq_set_name(cm_context->peer_cm_queue,"peer_cm_queue");
	cm_context->cm_operating_mode = UNKNOWN_CONNECTION_MODE;
	//cm_context->cm_operating_mode = PREEMPTIVE;
	cm_context->multicast_peer_client = NULL;
	memset(cm_context->audio_channels, CM_AV_CHANNEL_UNUSED, MAX_SHARED_CONNECTIONS);
	memset(cm_context->video_channels, CM_AV_CHANNEL_UNUSED, MAX_SHARED_CONNECTIONS);
	cm_context->resolution_change_needed[HEAD_1] = false;
	cm_context->resolution_change_needed[HEAD_2] = false;
	cm_context->statistics_counter = 60;

	//corrib_syslog(LOG_DEBUG,"%s: cm_context->peer_cm_queue = %p\n",__func__,cm_context->peer_cm_queue);
    LIST_INIT(&(cm_context->peer_list_head));                       /* Initialize the peer list. */
    TAILQ_INIT(&(cm_context->peer_wait_queue_head));   //for peers waiting to join
    pthread_mutex_init(&(cm_context->mutex), NULL);
	cm_context->performance_analysis = false;
	hw_manager_set_queues(cm_context->hm_context,cm_context->hm_cm_queue,cm_context->cm_hm_queue); //set the hw manager's queues
	connection_manager_init_slave_cid_pool(cm_context);
	return cm_context;
}


/* Destructor
 *
 */
void connection_manager_free(cmContext * cm_context)
{

	EventEnd* end_event =  event_end_new(); //create an end event to stop the hardware manager
	eq_push(cm_context->cm_hm_queue, (eqEvent *)end_event);
	while(cm_context->hm_context->main_thread_state == RUNNING) //wait for the manager to end
	{

		//corrib_syslog(LOG_DEBUG,"wait hm end\n");
		usleep(10);
	}
	corrib_syslog(LOG_INFO,"CM:Hardware Manager has ended with status: %d (%s)\n",cm_context->hm_context->hm_exit_state,cm_context->hm_context->exit_info);
#ifdef MEMORY_ALLOCATION_MONITOR
	eq_queue_free(cm_context->hm_cm_queue,__func__);
	eq_queue_free(cm_context->cm_hm_queue,__func__);
	eq_queue_free(cm_context->peer_cm_queue,__func__); //receives incomming events from the peers
#else
	eq_queue_free(cm_context->hm_cm_queue);
	eq_queue_free(cm_context->cm_hm_queue);
	eq_queue_free(cm_context->peer_cm_queue); //receives incomming events from the peers
#endif
	hw_manager_free(cm_context->hm_context);
	pthread_mutex_destroy(&(cm_context->mutex));
	xfree(cm_context,__func__);
	corrib_syslog(LOG_DEBUG,"CM:Connection Manager has ended\n");
}

/* Disable the client socket to trigger termination of the main or secondary threads
   as they call client->CheckFileDescriptor() on each iteration, which will fail
   after the socket is disabled here. */
static void connection_manager_terminate_peer(freerdp_peer * client)
{
	corrib_syslog(LOG_INFO, "CM:%s: removing %s \n", __func__, client->hostname);
	shutdown(client->sockfd, SHUT_RDWR);
}

//Access Functions
//---------------------------------------------

/*
 * Any queues here are created by the owner of this object, normally the listner
 */
void connection_manager_set_queues(cmContext * cm_context,eqEventQueue* listner_queue)
{

	cm_context->listner_queue = listner_queue;
}


static void connection_manager_set_exit_state(cmContext * cm_context,exitState state,const char * message)
{

	cm_context->hm_exit_state = state;
	strncpy(cm_context->exit_info,message,255);

}





void connection_manager_enable_performance_analysis(cmContext * cm_context)
{
	cm_context->performance_analysis = true;
	if(cm_context->hm_context)
		hw_manager_enable_performance_analysis(cm_context->hm_context);
}

void connection_manager_enable_debug(cmContext * cm_context)
{

	cm_context->debug_enabled = true;
	if(cm_context->hm_context)
		hw_manager_enable_debug(cm_context->hm_context);
}



//Main Processing Loop and thread management
//-------------------------------------------
void connection_manager_run(cmContext * cm_context)
{

	statistcs_send_json_control_object("flush_active_connections");

	if(su_toe_init(TRANSMITTER))
	{
		corrib_syslog(LOG_ERR,"CM: Failed to initialise TOE config, terminating\n");
		exit(0);
	}
	if(su_avae_init())
	{
		corrib_syslog(LOG_ERR,"CM: Failed to initialise AVAE config, terminating\n");
		exit(0);
	}

	cm_context->main_thread = connection_manager_create_thread(connection_manager_main_loop,cm_context);
	cm_context->video_municast_running = 0;
	cm_context->audio_municast_running = 0;
	cm_context->video_multicast_running = 0;
	cm_context->audio_multicast_running = 0;
	cm_context->connecting_client = NULL;
	if(cm_context->main_thread == -1)
	{
		corrib_syslog(LOG_ERR,"CM:Failed to create thread for connection_manager,terminating\n");
		exit(0);
	}
	if(freerdp_check_file_exists("/usr/local/FPGA_RESET_TEST"))
	{
		corrib_syslog(LOG_DEBUG,"Entering FPGA reset test mode\n");
		hw_manager_test_fpga_reset(cm_context->hm_context);
	}
	else
		hw_manager_run(cm_context->hm_context); //start the hardware manager

}

static pthread_t connection_manager_create_thread( void* func, void* arg)
{
	pthread_t thread;
	if(pthread_create(&thread, 0, func, arg) != 0)
	{
				corrib_syslog(LOG_ERR,"%s: Failed to created thread for connection_manager\n",__func__);
		return -1;
	}
	else
	{

		return thread;
	}
	return 0;
}


int connection_manager_check_fd_status(char * name,int file)
{

    struct stat fileStat;
    if(fstat(file,&fileStat) < 0)
        return 1;


    printf("%s ------\n",name);
    printf("File Size: \t\t%llu bytes\n",fileStat.st_size);
    printf("Number of Links: \t%d\n",fileStat.st_nlink);
    printf("File inode: \t\t%llu\n",fileStat.st_ino);
    printf("File Permissions: \t");
    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
    printf("The file %s a symbolic link\n\n", (S_ISLNK(fileStat.st_mode)) ? "is" : "is not");
    return 0;
}

static int connection_manager_netlink_init()
{
	int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);

	if(sock_fd < 0)
		return -1;

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); /* self pid */
	src_addr.nl_groups = 1;

	bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	return sock_fd;
}

static int connection_manager_netlink_process(int sock_fd)
{
	struct sockaddr_nl dest_addr;
	struct msghdr msg;
	struct iovec iov;
	struct nl_message leave;
	struct nlmsghdr * nlh = NULL;

	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 1;

	nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(NLINK_MSG_LEN));
	nlh->nlmsg_len = NLMSG_SPACE(NLINK_MSG_LEN);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&(dest_addr);
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	recvmsg(sock_fd, &msg, 0);

	memcpy(&leave, (struct nl_message *)NLMSG_DATA(nlh), sizeof(struct nl_message));
	free(nlh);

	return leave.cid;
}

/*
 * This needs to be capable of dynamically adding fds from multiple sources, for example peers who will come and go
 */
static boolean connection_manager_get_fds(cmContext * cm_context, void** rfds, int* rcount)
{
	//int i;
	if(*rcount > MAX_FDS)
	{
		corrib_syslog(LOG_ERR,"CM:connection manager has exceeded maximum file descriptors (%d) in %s\n", MAX_FDS, __func__);
		return false;
	}
	//get the queue FD for the hardware manager as we must process his events
	int fd_hm_cm = eq_get_queue_fd(cm_context->hm_cm_queue);
	if (fd_hm_cm < 1)
	{
		corrib_syslog(LOG_ERR,"CM:connection manager failed to get file descriptors for hm queue in %s\n",__func__);
		return false;
	}
	else
	{

		rfds[*rcount] = (void*)(long)(fd_hm_cm);
		(*rcount)++;
	}

	int fd_listner = eq_get_queue_fd(cm_context->listner_queue);
	if (fd_listner < 1)
	{
		corrib_syslog(LOG_ERR,"CM:connection manager failed to get file descriptors for listner queue in %s\n",__func__);
		return false;
	}
	else
	{

		rfds[*rcount] = (void*)(long)(fd_listner);
		(*rcount)++;
	}

	int fd_peer = eq_get_queue_fd(cm_context->peer_cm_queue);
	if (fd_peer < 1)
	{
		corrib_syslog(LOG_ERR,"CM:connection manager failed to get file descriptors for peer queue in %s\n",__func__);
		return false;
	}
	else
	{
		rfds[*rcount] = (void*)(long)(fd_peer);
		(*rcount)++;
	}

	if (cm_context->netlink_sock_fd < 1) {
		corrib_syslog(LOG_ERR,"CM:connection manager failed to get file descriptors for netlink sock in %s\n",__func__);
		return false;
	} else {
		/* fd for netlink socket */
		rfds[*rcount] = (void*)(long)(cm_context->netlink_sock_fd);
		(*rcount)++;
	}
/*
	for (i = 0; i < listener->num_sockfds; i++)
	{
		rfds[*rcount] = (void*)(long)(listener->sockfds[i]);
		(*rcount)++;
	}
*/
	return true;
}



void connection_manager_get_timeout_interval(cmContext * context, struct timeval* tv)
{
	if(context != NULL && tv != NULL)
	{
	    tv->tv_sec = CM_DEFAULT_INTERVAL_PERIOD;
	    tv->tv_usec = 0;

	}
}

static void connection_manager_update_video_rtt_stats(cmContext * cm_context,
						      boolean snapshot)
{
	int remaining_peers = connect_manager_get_peer_list_size(cm_context);
	peerNode * node_iterator;
	unsigned long rtt_current = 0UL;
	static unsigned long rtt_min = -1UL;
	static unsigned long rtt_max = 0UL;
	static unsigned long rtt_count = 0UL;
	static unsigned long long rtt_accum = 0ULL;

	if (!remaining_peers) {
		/* No clients connected, reset the counters and return */
		rtt_min = -1UL;
		rtt_max = 0UL;
		rtt_count = 0UL;
		rtt_accum = 0ULL;
		return;
	}

	/*
	 * Roll up the RTT stats from all active connections to provide overall
	 * RTT stats to accompany the video stats reported out to Boxilla
	 */
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if (node_iterator->client)
		{
			freerdp_peer *client = node_iterator->client;
			uint64_t rtt_usec;
			int err;

			/* Skip this client if a valid video channel hasn't yet been allocated */
			if (client->video_slave_cid <= 0)
				continue;

			err = su_toe_rtt_get(client->video_slave_cid, &rtt_usec);
			if (err)
			{
				corrib_syslog(LOG_WARNING, "%s: failed to get RTT from video CID %u\n",
					      __func__, client->video_slave_cid);
				continue;
			}

			rtt_accum += rtt_usec;
			rtt_count++;
			if (rtt_usec < rtt_min)
				rtt_min = rtt_usec;
			if (rtt_usec > rtt_max)
				rtt_max = rtt_usec;
			/* Select worst-case RTT value from active connections as "current" RTT */
			if (rtt_usec > rtt_current)
				rtt_current = rtt_usec;
		}
	}

	if (snapshot)
	{
		/* corrib_syslog(LOG_DEBUG, "%s: rtt=%u, min_rtt=%u, max_rtt=%u, avg_rtt=%u",
			      __func__, rtt_current, rtt_min, rtt_max, rtt_count ? (rtt_accum / rtt_count) : 0); */
		/* Snapshot the current rtt statistics */
		cm_context->hm_context->videoStatistics.rtt_usec = rtt_current;
		cm_context->hm_context->videoStatistics.min_rtt_usec = rtt_min;
		cm_context->hm_context->videoStatistics.max_rtt_usec = rtt_max;
		cm_context->hm_context->videoStatistics.avg_rtt_usec = rtt_count ? (rtt_accum / rtt_count) : 0;
		/* and reset the counters */
		rtt_min = -1UL;
		rtt_max = 0UL;
		rtt_count = 0UL;
		rtt_accum = 0ULL;
	}
}
void connection_manager_refresh_active_connection_stats(cmContext * cm_context)
{
	uint32_t optimised_peers = 0;
	uint32_t lossless_peers = 0;
	int remaining_peers = connect_manager_get_peer_list_size_and_types(cm_context, &optimised_peers, &lossless_peers);

	if(cm_context->statistics_counter++ > 60)
	{
#ifdef ENABLE_WEB_SERVICE_REPORTING
		statistcs_send_json_system_fact_object();
#endif
		cm_context->statistics_counter = 0;
	}

	if(remaining_peers)
	{
		uint32 current_time;
		uint32 time_difference;
		peerNode * node_iterator;
		current_time = sh_log_get_mstime();
		time_difference = current_time - cm_context->previous_interval_time;
		if(time_difference > 60000) // more than a minute has elapsed
		{
			/* Update RTT stats, save snapshot with averaged RTT, and reset */
			connection_manager_update_video_rtt_stats(cm_context, true);
			hw_manager_video_stats(cm_context->hm_context, optimised_peers, lossless_peers);
			hw_manager_analogaudio_stats(cm_context->hm_context);

			LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
			{
				if(node_iterator->client)
				{
					node_iterator->client->connection_duration++;
#ifdef ENABLE_WEB_SERVICE_REPORTING
					statistcs_send_json_active_connection_statistic_object(
						"unknown","unknown",
						node_iterator->client->connection_mode,node_iterator->client->connection_start_time,
						node_iterator->client->connection_id,node_iterator->client->connection_hostname,0);
#endif
				}
			}
			cm_context->previous_interval_time = current_time;
		}
		else
		{
			/* Update the accumated RTT stats counters for averaging later */
			connection_manager_update_video_rtt_stats(cm_context, false);
		}
	}
	else
	{
		/* Reset the RTT stats counters when no clients are connected */
		connection_manager_update_video_rtt_stats(cm_context, false);
	}
}

static int connection_manager_get_municast_channel(cmContext * cm_context, av_mode mode) {
	if (mode == VIDEO_MODE) {
		for (int i = 0; i < MAX_SHARED_CONNECTIONS; i++) {
			if (cm_context->video_channels[i] == CM_AV_CHANNEL_UNUSED) {
				cm_context->video_channels[i] = CM_AV_CHANNEL_ALLOCATED;
				return i;
			}
		}
	} else {
		for (int i = 0; i < MAX_SHARED_CONNECTIONS; i++) {
			if (cm_context->audio_channels[i] == CM_AV_CHANNEL_UNUSED) {
				cm_context->audio_channels[i] = CM_AV_CHANNEL_ALLOCATED;
				return i;
			}
		}
	}

	return -1;
}

static int connection_manager_remaining_municast_channel(cmContext * cm_context, av_mode mode)
{

	int index = 0;
	int remaining_peers = 0;

	if (mode == VIDEO_MODE) {
		for(index = 0; index < MAX_SHARED_CONNECTIONS; index++) {
			if (cm_context->video_channels[index] != CM_AV_CHANNEL_UNUSED)
				remaining_peers++;
		}
	} else {
		for(index = 0; index < MAX_SHARED_CONNECTIONS; index++) {
			if (cm_context->audio_channels[index] != CM_AV_CHANNEL_UNUSED)
				remaining_peers++;
		}
	}

	return remaining_peers;
}

static boolean connection_manager_channel_is_allocated(cmContext * cm_context, av_mode mode, int channel)
{
	if (channel < 0 || channel >= MAX_SHARED_CONNECTIONS)
		return false;

	return (mode == VIDEO_MODE) ?
		(cm_context->video_channels[channel] == CM_AV_CHANNEL_ALLOCATED) :
		(cm_context->audio_channels[channel] == CM_AV_CHANNEL_ALLOCATED);
}

static boolean connection_manager_channel_is_listening(cmContext * cm_context, av_mode mode, int channel)
{
	if (channel < 0 || channel >= MAX_SHARED_CONNECTIONS)
		return false;

	return (mode == VIDEO_MODE) ?
		(cm_context->video_channels[channel] == CM_AV_CHANNEL_LISTENING) :
		(cm_context->audio_channels[channel] == CM_AV_CHANNEL_LISTENING);
}

static boolean connection_manager_channel_is_active(cmContext * cm_context, av_mode mode, int channel)
{
	if (channel < 0 || channel >= MAX_SHARED_CONNECTIONS)
		return false;

	return (mode == VIDEO_MODE) ?
		(cm_context->video_channels[channel] == CM_AV_CHANNEL_ACTIVE) :
		(cm_context->audio_channels[channel] == CM_AV_CHANNEL_ACTIVE);
}

static int connection_manager_listening_municast_channels(cmContext *cm_context,
		av_mode mode) {

	int index = 0;
	int listening_channels = 0;

	for (index = 0; index < MAX_SHARED_CONNECTIONS; index++) {
		if (connection_manager_channel_is_listening(cm_context, mode, index)) {
			listening_channels ++;
		}
	}

	return listening_channels;
}

static int connection_manager_active_municast_channels(cmContext * cm_context, av_mode mode)
{

	int index = 0;
	int active_channels = 0;

	for (index = 0; index < MAX_SHARED_CONNECTIONS; index++)
		if (connection_manager_channel_is_active(cm_context, mode, index))
			active_channels++;

	return active_channels;
}

void connection_manager_set_server_mode(cmContext * cm_context, freerdp_peer * client)
{
	cm_context->cm_compression_mode |= client->compression_mode;
	cm_context->cm_operating_mode |= client->connection_mode;
	cm_context->hm_context->configured_compression = cm_context->cm_compression_mode;
}

static const char * cm_get_peer_video_state_string(const rdp_peer_video_state video_state)
{
	switch (video_state) 
	{
		case PEER_VIDEO_STATE_ACTIVE: 
			return "PEER_VIDEO_STATE_ACTIVE";
		case PEER_VIDEO_STATE_SYNC_LOSS: 
			return "PEER_VIDEO_STATE_SYNC_LOSS";
		case PEER_VIDEO_STATE_RES_CHANGE:
			return "PEER_VIDEO_STATE_RES_CHANGE";
		default:
			return "UNKNOWN_PEER_VIDEO_STATE";
	}
}

static const char * cm_get_peer_connection_state_string(const rdp_peer_connection_state connection_state)
{
	switch (connection_state) 
	{
		case PEER_CONNECTION_STATE_INIT: 
			return "PEER_CONNECTION_STATE_INIT";
		case PEER_CONNECTION_STATE_ACCEPTED: 
			return "PEER_CONNECTION_STATE_ACCEPTED";
		case PEER_CONNECTION_STATE_STARTING:
			return "PEER_CONNECTION_STATE_STARTING";
		case PEER_CONNECTION_STATE_RUNNING:
			return "PEER_CONNECTION_STATE_RUNNING";
		default:
			return "UNKNOWN_CONNECTION_STATE";
	}
}

//we use this to process tasks at low frequency intervals
void connection_manager_process_interval_tasks(cmContext * cm_context)
{
	peerNode * node_iterator;
	int remaining_peers = 0;
	int head = 0;
	//corrib_syslog(LOG_DEBUG,"%s:[CM]",__func__);

	/*
	 * BUG2580: TCP RTT values from the TOE are currently unreliable
	 * so, as a workaround, use the RTT measured from the RDP
	 * connection by Linux instead to periodically recalculate a
	 * suitable retransmission timeout (RTO) for the TOE fast-path
	 * connections.
	 *
	 * BUG2682: The TCP MSS may need to be adjusted depending
	 * on the MTU limits set along the network path. Utilise
	 * the Linux network stack to discover the maximum Path
	 * MTU and update the TOE MSS accordingly.
	 */

	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if (node_iterator->client)
			client_update_tcp_info(node_iterator->client, __func__);
	}

	if(cm_context->video_multicast_running) {
		client_update_multicast_info(cm_context, __func__);
	}

	if(freerdp_check_file_exists_and_delete("/tmp/ENABLE_CM_HEARTBEATS"))
	{
		cm_context->enable_cm_heartbeats = true;
        corrib_syslog(LOG_DEBUG,"CM:%s: Heartbeat enabled.\n",  __func__);
	}

	if(cm_context->enable_cm_heartbeats)
	{
        connection_manager_show_peer_list(cm_context);
    	cm_context->enable_cm_heartbeats = false;
	}
	connection_manager_refresh_active_connection_stats(cm_context);
	connection_manager_process_wait_queue(cm_context);

	for (head = 1; head <= 2; head++) {
		if (cm_context->resolution_change_needed[head-1]) {
			remaining_peers = connect_manager_get_peer_list_size(cm_context);

			boolean all_clients_active = true;

			LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries) 
			{
				if (!node_iterator->client->is_active[head-1]) 
				{
					all_clients_active = false;
					break;
				}
				corrib_syslog(LOG_DEBUG,"%s: %s head %d is_active is %d, video_state is %s\n",  __func__, node_iterator->client->connection_hostname, head-1, 
				node_iterator->client->is_active[head-1], cm_get_peer_video_state_string(node_iterator->client->video_state[head-1]));
			
			}

			/* Wait for all Clients to Respond to the res change before 
			turning back on */ 
			if (all_clients_active) 
			{
				corrib_syslog_bs (LOG_INFO,"cm_process_interval_tasks");
				//Track if all clients are finished and ready
				boolean restart_capture = true;
				cm_context->resolution_change_needed[head-1] = false;

				LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries) 
				{
					/* Don't forward res complete feedback for 2nd head on 
					* a single head client */
					if (head == 2 && node_iterator->client->settings->num_monitors == 0)
						continue;

					restart_capture &= connection_manager_handle_res_change_complete(cm_context,
											node_iterator->client->sockfd, head);
				}

				if(restart_capture)
				{
					hardware_manager_start_capture_subsystem(cm_context->hm_context, head-1, cm_context->cm_compression_mode);
				}
				else
				{
					corrib_syslog(LOG_DEBUG,"%s: Not Restarting Capture we have missing events\n",  __func__);
				}
				corrib_syslog_es (LOG_INFO,"cm_process_interval_tasks");
			}
		}
	}
    // #ifdef VERBOSE_DEBUGGING
    // corrib_syslog(LOG_DEBUG,"CM:%s: Ran interval tasks.\n",  __func__);
    // #endif
}

static void connection_manager_arbitrate_keyboard_mouse_control(cmContext * cm_context, int peer_id, int exclusive_mode_request)
{


	if(exclusive_mode_request)
	{
		corrib_syslog(LOG_DEBUG,"%s EXCLUSIVE MODE ARBITRATION REQUEST, cm_context->cm_operating_mode : %d",__func__, cm_context->cm_operating_mode);
		// if keys are held force them up
		if(cm_context->hm_context->keys_held > 0 )
		{
			eq_purge(EQ_EVENT_MOUSE, cm_context->cm_hm_queue);
			eq_purge(EQ_EVENT_KEYBOARD, cm_context->cm_hm_queue);
			hw_manager_flush_pressed_keys(cm_context->hm_context);
			cm_context->hm_context->keys_held = 0;
		}

		cm_context->controlling_peer_id = peer_id;
	}

	if(!MODE_CHECK(cm_context->cm_operating_mode, EXCLUSIVE_MODE))
	{
		int timeout;
		uint32 current_time;

		current_time = sh_log_get_mstime();

		if (cm_context->mouse_keyboard_timeout == 0)
			timeout = 100;
		else
			timeout = cm_context->mouse_keyboard_timeout * 1000;

		if(cm_context->hm_context->keys_held < 0)
			cm_context->hm_context->keys_held = 0;

		if((current_time - (cm_context->mouse_keyboard_timer) > timeout) && cm_context->hm_context->keys_held == 0)
			cm_context->mouse_keyboard_available = true;

		if(cm_context->controlling_peer_id == peer_id)
			cm_context->mouse_keyboard_timer = current_time;

		if(cm_context->mouse_keyboard_available)
		{
			cm_context->mouse_keyboard_available = false;
			cm_context->controlling_peer_id = peer_id;
		}
	}
}


static void connection_manager_process_wait_queue(cmContext * cm_context)
{
	unsigned waiting_count = 0;
	peerWaitNode * node_iterator;
	TAILQ_FOREACH(node_iterator, &(cm_context->peer_wait_queue_head), entries)
	{
		if (node_iterator)
		{
			/*
			 * Launch only one new client at a time to avoid
			 * multiple clients connecting to the same TOE
			 * listening port at once
			 */
			if (! cm_context->connecting_client)
			{
				corrib_syslog(LOG_INFO,"CM:%s: Launching deferred connection for %s\n",
					      __func__,node_iterator->hostname);
				connection_manager_handle_new_client_request(cm_context,node_iterator->fd, node_iterator->hostname);
				TAILQ_REMOVE(&(cm_context->peer_wait_queue_head), node_iterator, entries);
				xfree(node_iterator,__func__);
			} else {
				waiting_count++;
			}
		}
	}
	if (waiting_count)
		corrib_syslog(LOG_INFO,"CM:%s: %u clients waiting to connect\n",
			      __func__, waiting_count);
}



static void connection_manager_init_slave_cid_pool(cmContext * cm_context)
{

	int index = 0;
	unsigned int Vslave_cid = STARTING_MULTICAST_VIDEO_CID;
	unsigned int Aslave_cid = STARTING_MULTICAST_AUDIO_CID;
	for(index = 0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		cm_context->video_slave_cid_pool[index][0] = Vslave_cid++;
		cm_context->video_slave_cid_pool[index][1] = 0;
		cm_context->audio_slave_cid_pool[index][0] = Aslave_cid++;
		cm_context->audio_slave_cid_pool[index][1] = 0;
	}
}

static void connection_manager_show_slave_cid_pool(cmContext * cm_context)
{

	int index = 0;
	printf("Showing video slave pools\n*****************\n");
	for(index =0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		printf("%d:VIDEO CID=%02x, in use=%u\n",index,cm_context->video_slave_cid_pool[index][0],cm_context->video_slave_cid_pool[index][1]);
	}
	printf("Showing audio slave pools\n*****************\n");
	for(index =0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		printf("%d:AUDIO CID=%02x, in use=%u\n",index,cm_context->audio_slave_cid_pool[index][0],cm_context->audio_slave_cid_pool[index][1]);
	}
}

static void connection_manager_show_video_slave_cid_pool(cmContext * cm_context)
{

	int index = 0;
	printf("*************Showing video slave pools in use \n*****************\n");
	for(index =0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		if(cm_context->video_slave_cid_pool[index][1] == 1)
			printf("%d:VIDEO CID=%02x, in use=%u\n",index,cm_context->video_slave_cid_pool[index][0],cm_context->video_slave_cid_pool[index][1]);
	}
}

static freerdp_peer* connection_manager_cid_to_client(cmContext *cm_context, unsigned cid)
{
	peerNode * node_iterator;

	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if (node_iterator->client->video_slave_cid == cid ||
				node_iterator->client->audio_slave_cid == cid) {
			corrib_syslog(LOG_WARNING, "Tearing Client: %s with MU ID %d and Video CID=%02X\n",
					node_iterator->client->hostname,
					node_iterator->client->video_channel,
					node_iterator->client->video_slave_cid);
			return node_iterator->client;
		}
	}

	corrib_syslog(LOG_WARNING, "Could not find matching client with CID=%02X\n", cid);
	return NULL;
}

static unsigned connection_manager_video_channel_to_cid(unsigned video_channel)
{
	unsigned cid = 0;

	if (video_channel < MAX_SHARED_CONNECTIONS)
		cid = STARTING_MUNICAST_VIDEO_CID + video_channel;
	else
		corrib_syslog(LOG_ERR, "%s: Invalid video channel %u specified\n",
			      __func__, video_channel);

	return cid;
}

static unsigned connection_manager_audio_channel_to_cid(unsigned audio_channel)
{
	unsigned cid = 0;

	if (audio_channel < MAX_SHARED_CONNECTIONS)
		cid = STARTING_MUNICAST_AUDIO_CID + audio_channel;
	else
		corrib_syslog(LOG_ERR, "%s: Invalid audio channel %u specified\n",
				__func__, audio_channel);

	return cid;
}

static int connection_manager_check_remaining_video_slave(cmContext * cm_context)
{

	int index = 0;
	int remaining_peers = 0;

	for(index =0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		if (cm_context->video_slave_cid_pool[index][1])
			remaining_peers++;
	}

	return remaining_peers;
}

static int connection_manager_check_remaining_audio_slave(cmContext * cm_context)
{

	int index = 0;
	int remaining_peers = 0;

	for(index =0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		if (cm_context->audio_slave_cid_pool[index][1])
			remaining_peers++;
	}

	return remaining_peers;
}

static int connection_manager_replenish_slave_cid_pool(cmContext * cm_context,freerdp_peer* client, av_mode mode)
{
	corrib_syslog_bs (LOG_INFO,"cm_replenish_slave_pool");
	int index = 0;
	uint32 slave_cid;
	int *PoolPtr;

	if (mode == VIDEO_MODE) {
		PoolPtr = &cm_context->video_slave_cid_pool[0][0];
		slave_cid = client->video_slave_cid;

		if ((slave_cid < STARTING_MULTICAST_VIDEO_CID) ||
		    (slave_cid >= (STARTING_MULTICAST_VIDEO_CID + MAX_SHARED_CONNECTIONS))) {
			corrib_syslog(LOG_ERR,"%s: Invalid Video Slave CID[%02x] for %s.\n",
				      __func__, slave_cid, client->hostname);
                corrib_syslog_es (LOG_INFO,"cm_replenish_slave_pool");
			return 0;
		}
	} else {
		PoolPtr = &cm_context->audio_slave_cid_pool[0][0];
		slave_cid = client->audio_slave_cid;

		if ((slave_cid < STARTING_MULTICAST_AUDIO_CID) ||
		    (slave_cid >= (STARTING_MULTICAST_AUDIO_CID + MAX_SHARED_CONNECTIONS))) {
			corrib_syslog(LOG_ERR,"%s: Invalid Audio Slave CID[%02x] for %s.\n",
				      __func__, slave_cid, client->hostname);
                corrib_syslog_es (LOG_INFO,"cm_replenish_slave_pool");          
			return 0;
		}
	}

#ifdef TRACE_NEGOTIATION
	corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u %s Mode=%u Releasing CT%u\n",client->connection_id,__func__,mode,slave_cid);
#endif
	for(index = 0; index < MAX_SHARED_CONNECTIONS; index++)
	{
		//corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u comparing %u to %u\n",client->connection_id,slave_cid,*(PoolPtr + (index * MAX_CLOUMNS + 0)));
		if(slave_cid == *(PoolPtr + (index * MAX_CLOUMNS + 0)))
		{
			*(PoolPtr + (index * MAX_CLOUMNS + 1)) = 0;
#ifdef TRACE_NEGOTIATION
			corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u relinquished CT%u %u\n",client->connection_id,slave_cid,*(PoolPtr + (index * MAX_CLOUMNS + 0)));
#endif
#ifdef SHARED_MODE_DEBUG
			connection_manager_show_video_slave_cid_pool(cm_context);
            
#endif
            corrib_syslog_es (LOG_INFO,"cm_replenish_slave_pool");
			return 1;
		}
	}
#ifdef TRACE_NEGOTIATION
	corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u: %s Mode=%u Failed to find match for %u\n",client->connection_id,__func__,mode,slave_cid);
#endif

	connection_manager_show_video_slave_cid_pool(cm_context);
	corrib_syslog_es (LOG_INFO,"cm_replenish_slave_pool");
	return 0;
}

void connection_manager_channel_set_active(cmContext *cm_context, av_mode mode, int index) {
	if (mode == VIDEO_MODE) {
		if (cm_context->video_channels[index] != CM_AV_CHANNEL_LISTENING)
			corrib_syslog(LOG_ERR, "%s:video channel %d in incorrect state %d, not activating\n",
				      __func__, index, cm_context->video_channels[index]);
		else
			cm_context->video_channels[index] = CM_AV_CHANNEL_ACTIVE;
	} else {
		if (cm_context->audio_channels[index] != CM_AV_CHANNEL_LISTENING)
			corrib_syslog(LOG_ERR, "%s:audio channel %d in incorrect state %d, not activating\n",
				      __func__, index, cm_context->audio_channels[index]);
		else
			cm_context->audio_channels[index] = CM_AV_CHANNEL_ACTIVE;
	}
}

void connection_manager_channel_set_listening(cmContext *cm_context, av_mode mode, int index) {
	if (mode == VIDEO_MODE) {
		if (cm_context->video_channels[index] != CM_AV_CHANNEL_ALLOCATED)
			corrib_syslog(LOG_ERR, "%s:video channel %d in incorrect state %d, not changing to listening\n",
				      __func__, index, cm_context->video_channels[index]);
		else
			cm_context->video_channels[index] = CM_AV_CHANNEL_LISTENING;
	} else {
		if (cm_context->audio_channels[index] != CM_AV_CHANNEL_ALLOCATED)
			corrib_syslog(LOG_ERR, "%s:audio channel %d in incorrect state %d, not changing to listening\n",
				      __func__, index, cm_context->audio_channels[index]);
		else
			cm_context->audio_channels[index] = CM_AV_CHANNEL_LISTENING;
	}
}

void connection_manager_channel_set_inactive(cmContext *cm_context, av_mode mode, int index) {
	if (mode == VIDEO_MODE) {
		if (cm_context->video_channels[index] != CM_AV_CHANNEL_ACTIVE)
			corrib_syslog(LOG_ERR, "%s:video channel %d in incorrect state %d, not deactivating\n",
				      __func__, index, cm_context->video_channels[index]);
		else
			cm_context->video_channels[index] = CM_AV_CHANNEL_ALLOCATED;
	} else {
		if (cm_context->audio_channels[index] != CM_AV_CHANNEL_ACTIVE)
			corrib_syslog(LOG_ERR, "%s:audio channel %d in incorrect state %d, not deactivating\n",
				      __func__, index, cm_context->audio_channels[index]);
		else
			cm_context->audio_channels[index] = CM_AV_CHANNEL_ALLOCATED;
	}
}

void connection_manager_channel_set_unused(cmContext *cm_context, av_mode mode, int index) {
	if (mode == VIDEO_MODE) {
		if (cm_context->video_channels[index] == CM_AV_CHANNEL_UNUSED)
			corrib_syslog(LOG_ERR, "%s:video channel %d already in UNUSED state, not resetting\n",
				      __func__, index);
		else
			cm_context->video_channels[index] = CM_AV_CHANNEL_UNUSED;
	} else {
		if (cm_context->audio_channels[index] == CM_AV_CHANNEL_UNUSED)
			corrib_syslog(LOG_ERR, "%s:audio channel %d already in UNUSED state, not resetting\n",
				      __func__, index);
		else
			cm_context->audio_channels[index] = CM_AV_CHANNEL_UNUSED;
	}
}

/* Description: Assign Slave ID
 * Pramas: Pointer to cm_context
 * 	   Mode: 0:Video 1:Audio
*/
unsigned int connection_manager_get_slave_cid(cmContext * cm_context, av_mode mode,freerdp_peer * client)
{

	int index = 0;
	int *PoolPtr;

	if (mode == VIDEO_MODE)
		PoolPtr = &cm_context->video_slave_cid_pool[0][0];
	else
		PoolPtr = &cm_context->audio_slave_cid_pool[0][0];

	for(index = 0; index < MAX_SHARED_CONNECTIONS; index++)
	{
#ifdef TRACE_NEGOTIATION
		corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u %s Mode=%s Requesting slave id checking %u [%u]\n",client->connection_id,__func__, (mode == VIDEO_MODE)?"video":"audio",*(PoolPtr + (index * MAX_CLOUMNS + 0)),*(PoolPtr + (index * MAX_CLOUMNS + 1)));
#endif
		if( *(PoolPtr + (index * MAX_CLOUMNS + 1)) == 0 )
		{
			*(PoolPtr + (index * MAX_CLOUMNS + 1)) = 1; //we could write the peer_id here as an alternative
			corrib_syslog(LOG_DEBUG,"CM:%s: Allocating free %s_slave_cid =CT%02x\n", __func__, (mode == VIDEO_MODE)?"video":"audio" ,*(PoolPtr + (index * MAX_CLOUMNS + 0)));
#ifdef TRACE_NEGOTIATION
			corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u %s: Allocating free %s_slave_cid =CT%02x\n",
					client->connection_id, __func__, (mode == VIDEO_MODE)?"video":"audio" ,*(PoolPtr + (index * MAX_CLOUMNS + 0)));
#endif
			return *(PoolPtr + (index * MAX_CLOUMNS + 0));
		}
	}
	corrib_syslog(LOG_ERR,"CM:%s:No remaining %s_slave_cids\n", __func__, (mode == VIDEO_MODE)?"video":"audio");
	return 0; //we are out of slave CIDs
}



/*
 * The connection manager must monitor multiple queues, one from the listner, one from the hardware manager, and one from each peer
 */
static void * connection_manager_main_loop(void * arg)
{

	cmContext * cm_context = (cmContext *)arg;
	boolean running = true;
	int i;
	int fds;
	int max_fds;
	int rcount;
	void* rfds[32];
	fd_set rfds_set; //read fds
	fd_set efds_set; //error fds
	int num_set;
	int cid;
	memset(rfds, 0, sizeof(rfds));
	cm_context->main_thread_state = RUNNING;
	time_t last_processed = time(NULL);
	struct timeval tv = {.tv_sec = CM_DEFAULT_INTERVAL_PERIOD, .tv_usec = 0}; //

	cm_context->netlink_sock_fd = connection_manager_netlink_init();

	while(running)
	{

		rcount = 0;
		//corrib_syslog(LOG_DEBUG,"%s:[CM_ML]",__func__);
		int listner_fd = eq_get_queue_fd(cm_context->listner_queue);
		int hm_cm_fd = eq_get_queue_fd(cm_context->hm_cm_queue);
		int peer_cm_fd = eq_get_queue_fd(cm_context->peer_cm_queue);
		connection_manager_get_timeout_interval(cm_context, &tv);

		if(connection_manager_get_fds(cm_context, rfds, &rcount) != true)
		{
			corrib_syslog(LOG_ERR,"CM:Failed to get connection manager file descriptors in %s\n",__func__);
			running = false;
			cm_context->main_thread_state = STOPPED;

			break;
		}


		max_fds = 0;
		FD_ZERO(&rfds_set);

		for (i = 0; i < rcount; i++)
		{
			fds = (int)(long)(rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		if (max_fds == 0)
		{
			running = false;
			cm_context->main_thread_state = STOPPED;
			corrib_syslog(LOG_ERR,"CM:max fds are zero in %s\n",__func__);
			break;
		}


		//corrib_syslog(LOG_DEBUG,"CM_BS\n");
		num_set = select(max_fds + 1, &rfds_set, NULL, &efds_set, &tv);
		//corrib_syslog(LOG_DEBUG,"CM_AS\n");
		if(num_set == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				corrib_syslog(LOG_ERR,"CM:%s: select failed on error: %s.\n",  __func__,strerror(errno));
				running = false;
				cm_context->main_thread_state = STOPPED;
				connection_manager_check_fd_status("hm_cm_fd",hm_cm_fd);
				connection_manager_check_fd_status("listner_fd",listner_fd);
				connection_manager_check_fd_status("peer_cm_fd",peer_cm_fd);
				connection_manager_check_fd_status("Netlink_Sock",cm_context->netlink_sock_fd);
				break;
			}
		} //else everything is as we expected
		else
		{
			boolean known_event = false;
			EventNewConnection * new_connection_event;


			if (FD_ISSET(cm_context->netlink_sock_fd, &rfds_set)) //or if
			{
				known_event = true;
				cid = connection_manager_netlink_process(cm_context->netlink_sock_fd);
				freerdp_peer * client = connection_manager_cid_to_client(cm_context, cid);
				if (client) {
					if (MODE_CHECK(client->connection_mode, MULTICAST_MODE) )
					{
						su_toe_dump_connection_table(VIDEO_MASTER_CID, "VIDEO MASTER CHANNEL");
						if (client->settings->AnalogAudio) 
						{
							su_toe_dump_connection_table(AUDIO_MASTER_CID, "AUDIO MASTER CHANNEL");
						}
					}

					su_toe_dump_connection_table(client->video_slave_cid, "VIDEO CHANNEL");
					if (client->settings->AnalogAudio) 
					{
						su_toe_dump_connection_table(client->audio_slave_cid, "AUDIO CHANNEL");
					}

					connection_manager_terminate_peer(client);
				}
			}
			if (FD_ISSET(peer_cm_fd, &rfds_set)) //or if
			{
				known_event = true;
                                #ifdef VERBOSE_DEBUGGING
                                corrib_syslog(LOG_DEBUG,"CM:%s: checking for events from the peer.\n",  __func__);
                                #endif
				//read the queue
				eqEvent* event = eq_pop(cm_context->peer_cm_queue);

				if(event)
				{

					switch(event->type)
					{
					case EQ_EVENT_MOUSE: //we need to arbitrate this event on arrival before sending it on. FIXME
					{
						//we need to decide if this is the currently controlling connection
						//for now we just shunt it on to the hardware manager
						//it is the responsibility of the hardware manager to delete the event
						EventMouse* event_mouse = (EventMouse*)event;
						//corrib_syslog(LOG_DEBUG,"In MOuse Evnt Process peer id is %d\n__________\n",event_mouse->peer_id);
						int exclusive_request = false;
						connection_manager_arbitrate_keyboard_mouse_control(cm_context,event_mouse->peer_id, exclusive_request);
						int controlling_peer= cm_context->controlling_peer_id;
						if(controlling_peer == -1) //this is an error we must have a controlling peer
						{
							corrib_syslog(LOG_ERR,"CM:%s: Could not find a controlling peer.\n",  __func__);
							eq_event_free(event); //free the event
						}
						else
						{
							//corrib_syslog(LOG_INFO,"%s: controlling peer is %d.\n",  __func__,controlling_peer);
						}

						if(controlling_peer == event_mouse->peer_id)
						{
							if(cm_context->debug_enabled)
								corrib_syslog(LOG_DEBUG,"CM:%s: Forwarding mouse event to HM.\n",  __func__);
							if(0) //this is thr prefered way
								eq_push(cm_context->cm_hm_queue,event);
							else //this is an optimization for now we will still need to arbitrate the mouse in multicast so this will be done in CM
							{
								//if(cm_context->hm_context->suspend_video_h1 == false)
								if (cm_context->hm_context->enable_hid_tracing)
									corrib_syslog(LOG_DEBUG, "%s(): Mouse event received - x: %d y: %d flags: 0x%04x\n",
											__func__,
											((EventMouse *)event)->x,
											((EventMouse *)event)->y,
											((EventMouse *)event)->flags);
								hid_interface_process_mouse_event(cm_context->hm_context,event);
								eq_event_free(event); //free the event
							}

						}
						else
						{
							//corrib_syslog(LOG_INFO,"%s: Not Forwarding mouse event to HM. You are not the controlling peer\n",  __func__);
						}


						break;
					}
					case EQ_EVENT_KEYBOARD: //we need to arbitrate this event on arrival before sending it on. FIXME
					{
						//we need to decide if this is the currently controlling connection
						//for now we just shunt it on to the hardware manager
						//it is the responsibility of the hardware manager to delete the event
						EventKeyboard* event_keyboard = (EventKeyboard*)event;
						//corrib_syslog(LOG_DEBUG,"In Keybaord Evnt Process peer id is %d\n__________\n",event_keyboard->peer_id);
						int exclusive_request = false;
						connection_manager_arbitrate_keyboard_mouse_control(cm_context,event_keyboard->peer_id, exclusive_request);
						int controlling_peer= cm_context->controlling_peer_id;
						if(controlling_peer == -1) //this is an error we must have a controlling peer
						{
							corrib_syslog(LOG_ERR,"CM:%s: Could not find a controlling peer for keyboard.\n",  __func__);
							eq_event_free(event); //free the event
						}
						else
						{
							//corrib_syslog(LOG_INFO,"%s: controlling peer is %d.\n",  __func__,controlling_peer);
						}

						if(controlling_peer == event_keyboard->peer_id)
						{
							freerdp_peer* client = connection_manager_get_peer_for_fd(cm_context,event_keyboard->peer_id);
                            eq_push(cm_context->cm_hm_queue,event);
						}
						else
						{
							//corrib_syslog(LOG_INFO,"%s: Not Forwarding keyboard event to HM. You are not the controlling peer\n",  __func__);
						}
						break;
					}
					case EQ_EVENT_DESKTOP_RESIZE:  //this event needs to be sent to all active peers

						//FIXME, not so sure now that the CM needs to receive this from a peer

						break;
					case EQ_EVENT_CLIENT_SIDE_READY: //There is redundancy between this and the next event, we should remove one
					{
						EventClientReady * client_ready_event =  (EventClientReady *)event;
						hardware_manager_enable_all_tiles_mode(cm_context->hm_context, cm_context->cm_compression_mode);
						//the client is ready turn back on video
						if(cm_context->debug_enabled)
						{
							corrib_syslog(LOG_DEBUG,"CM:%s: client %d side ready.\n",  __func__,client_ready_event->client_id);
						}
						connection_manager_handle_keyboard_output_report(cm_context,cm_context->hm_context->outputReportBitmask);

						connection_manager_handle_client_ready(cm_context, client_ready_event->client_id,
								client_ready_event->connection_type, client_ready_event->preemption_requested,
								client_ready_event->domain_key, client_ready_event->session_id,client_ready_event->loggedin_user);
						//su_show_domain_key(client_ready_event->domain_key, __func__);

						eq_push(cm_context->cm_hm_queue,event);
						//Event will be freed in the Hardware Manager
						break;
					}
					case EQ_EVENT_RES_CHANGE_COMPLETE:
					{
						EventResChangeComplete * client_res_change_complete =  (EventResChangeComplete *)event;
						int hw_head_id = (client_res_change_complete->head == 1) ? HEAD_ONE : HEAD_TWO;
						//corrib_syslog(LOG_DEBUG,"%s: Client %d Resolution Change Complete.\n",  __func__,client_ready_event->client_id);
						//the client is ready turn back on video
						//if(cm_context->debug_enabled)
						corrib_syslog(LOG_INFO,"%s: Client %d Resolution Change Complete.\n",  __func__,client_res_change_complete->client_id);
						//Now update the hardware manager to un-suspend the video, this may already have been unsuspended during a resize exchange
						connection_manager_handle_keyboard_output_report(cm_context,cm_context->hm_context->outputReportBitmask);
						if(connection_manager_handle_res_change_complete(cm_context,client_res_change_complete->client_id, client_res_change_complete->head))
						{
							hardware_manager_start_capture_subsystem(cm_context->hm_context, client_res_change_complete->head-1, cm_context->cm_compression_mode);
						}
						hw_manager_set_media_suspend_state(cm_context->hm_context, hw_head_id, false);
						eq_event_free(event); //we need to free the event as it is travelling no further

						break;
					}
					case EQ_EVENT_CONNECTION_INFO:
					{
						EventConnectionInfo * client_connection_info =  (EventConnectionInfo *)event;
						corrib_syslog(LOG_INFO,"%s: Client %d connection info.\n",  __func__,client_connection_info->client_id);
						connection_manager_handle_keyboard_output_report(cm_context,cm_context->hm_context->outputReportBitmask);
						connection_manager_handle_connection_info(cm_context,client_connection_info);
						eq_event_free(event); //we need to free the event as it is travelling no further

						break;
					}
					case EQ_EVENT_SERVER_PEER_READY:
					{
						EventServerPeerReady * server_peer_ready_event =  (EventServerPeerReady *)event;
						boolean accepted;

						//the client is ready turn back on video
						if(cm_context->debug_enabled)
						{
							corrib_syslog(LOG_DEBUG,"%s: Server Peer ready.\n",  __func__);
						}
						accepted = connection_manager_handle_server_peer_ready(cm_context,server_peer_ready_event->client);
						if (accepted)
						{
							hardware_manager_enable_all_tiles_mode(cm_context->hm_context, cm_context->cm_compression_mode);
						}
						eq_event_free(event); //we need to free the event as it is travelling no further
						break;
					}
					case EQ_EVENT_CHANNEL_READY:
					{
						EventChannelReady* channel_ready_event = (EventChannelReady*)event;
						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"CM:%s: Audio Channel Ready.\n",  __func__);

						if (strncmp(channel_ready_event->channel->name, "rdpsnd", 6) == 0)
						{
							hw_manager_set_media_suspend_state(cm_context->hm_context,AUDIO,false); //FIXME, what about second head
						}
						eq_event_free(event); //we need to free the event as it is travelling no further
						break;
					}
					case EQ_EVENT_USB_COMMAND_AVAILABLE:
					{

						eq_push(cm_context->cm_hm_queue,event);

						break;
					}
					case EQ_EVENT_PEER_TERMINATED:  //a peer has left the building
					{
						//we need to clean up remove the peer from any active lists
						//and if necessary remove the peer from any multicast transport level lists
						EventPeerTerminated * peer_terminated_event = (EventPeerTerminated *)event;
						connection_manager_handle_peer_termination(cm_context, peer_terminated_event->peer_id);
						eq_event_free(event); //we need to free the event as it is travelling no further

						break;
					}
					default:
						corrib_syslog(LOG_ERR,"CM:%s: got an unknown event type from peer:%d.\n",  __func__,event->type);
						break;
					}
					//Note: we do not free the event here as it is being forwarded to the hardware manager
					//this may not be the case for other events


					//FIXME, ensure we free other events
				}
                               #ifdef VERBOSE_DEBUGGING
                               corrib_syslog(LOG_DEBUG,"CM:%s: checked for events from the peer.\n",  __func__);
                               #endif
			}

			if (FD_ISSET(listner_fd, &rfds_set)) //need to figure out which queue has fired
			{
				//read the queue
                                #ifdef VERBOSE_DEBUGGING
                                corrib_syslog(LOG_DEBUG,"CM:%s: checking    for events from the listner.\n",  __func__);
                                #endif
				known_event = true;
				eqEvent* event = eq_pop(cm_context->listner_queue);
				if(event)
				{
					switch(event->type)
					{
					case EQ_EVENT_END:
						corrib_syslog(LOG_INFO,"CM:%s: got an end event (%s), terminating.\n",  __func__,strerror(errno));
						running = false;
						cm_context->main_thread_state = STOPPED;
						break;
					case EQ_EVENT_NEW_CONNECTION:
						new_connection_event = (EventNewConnection *)event;
						if(cm_context->connecting_client)
						{
							peerWaitNode * node = xmalloc(sizeof(peerWaitNode),__func__);      /* Insert at the head. */
							strcpy(node->hostname,new_connection_event->hostname);
							node->fd=new_connection_event->peer_sockfd;
							TAILQ_INSERT_TAIL(&(cm_context->peer_wait_queue_head), node, entries);
							corrib_syslog(LOG_INFO,"CM:%s: Deferring a new connection event for client:%s (%d).\n",  __func__,new_connection_event->hostname,new_connection_event->peer_sockfd);
						}
						else
						{
							corrib_syslog(LOG_INFO,"CM:%s: Processing a new connection event for client:%s (%d).\n",  __func__,new_connection_event->hostname,new_connection_event->peer_sockfd);
							connection_manager_handle_new_client_request(cm_context,new_connection_event->peer_sockfd,new_connection_event->hostname);
						}
						break;
					default:
						corrib_syslog(LOG_ERR,"CM:%s: got an unknown event type from peer: %u.\n",  __func__,event->type);
						eq_show_event_type(event->type);
						break;
					}
					eq_event_free(event); //free the event
				}
                else
                {
                    corrib_syslog(LOG_ERR,"CM:%s: expected an event from the listner, but did not receive one: .\n",  __func__);
                }
			}
			if (FD_ISSET(hm_cm_fd, &rfds_set)) //or if
			{
                               #ifdef VERBOSE_DEBUGGING
                               corrib_syslog(LOG_DEBUG,"CM:%s: checking for events from the hardware Manager.\n",  __func__);
                               #endif

				known_event = true;
				//read the queue
				eqEvent* event = eq_pop(cm_context->hm_cm_queue);

				if(event)
				{
					switch(event->type)
					{
					case EQ_EVENT_END: //FIXME, in general the hardware manager should not stop the connection manager
						corrib_syslog(LOG_DEBUG,"CM:%s: got and end event, terminating: %s",  __func__,strerror(errno));
						running = false;
						cm_context->main_thread_state = STOPPED;
						connection_manager_set_exit_state(cm_context,NORMAL,"Normal exit, no errors\n");
						eq_event_free(event);
						break;
					case EQ_EVENT_REPORT_FAULT:
						corrib_syslog(LOG_DEBUG,"CM:%s: got a fault event (%s), reporting.\n",  __func__,strerror(errno));
						//corrib_syslog(LOG_DEBUG,"%s: got a fault event, reporting.\n",  __func__,strerror(errno));
						connection_manager_handle_fault_report(cm_context);
						eq_event_free(event);
						break;
					case EQ_EVENT_AUDIO_COMMAND_AVAILABLE:
					{
						EventAudioCommandAvailable * audio_command_available_event;
						audio_command_available_event = (EventAudioCommandAvailable *)event;

						audio_command_available_event->previous_frame_receive_time = cm_context->last_audio_command_available;
						cm_context->last_audio_command_available = event->receive_time;

						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"CM:%s: got an audio command available event \n",  __func__);

						//this now needs to go to the currently active peer
						connection_manager_handle_audio_command(cm_context, audio_command_available_event);
						break;
					}
					case EQ_EVENT_USB_COMMAND_AVAILABLE:
					{
						EventUsbCommandAvailable* usb_command_available_event;
						usb_command_available_event = (EventUsbCommandAvailable*)event;
						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"CM:%s: got an usb command available event \n",  __func__);

						//this now needs to go to the currently active peer
						connection_manager_handle_usb_command(cm_context, usb_command_available_event);
						break;
					}
					case EQ_EVENT_RESOLUTION_CHANGE:
					{
						EventResolutionChange * resolution_change_event;
						resolution_change_event = (EventResolutionChange *)event;

						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"CM:%s: got a resolution change event for head:%d\n",  __func__,resolution_change_event->head_id);

						connection_manager_handle_resolution_change(cm_context,resolution_change_event->head_id, resolution_change_event->connectionResData);

						eq_event_free(event); //FIXME, decide who frees this memory
						break;
					}
					case EQ_EVENT_SYNC_LOSS: //a sync loss from the hardware manager
					{
						EventSyncLoss * sync_loss_event;
						sync_loss_event = (EventSyncLoss *)event;

						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"CM:%s: got a resolution change event for head:%d\n",  __func__,sync_loss_event->head_id);

						//FIXME, we need to figure out how this is handled by all of the clients
						connection_manager_handle_sync_loss(cm_context,sync_loss_event->head_id);
						eq_event_free(event); //FIXME, decide who frees this memory
						break;
					}
					case EQ_EVENT_KEYBOARD_OUTPUT_REPORT:
					{
						EventKeyboardOutputReport* event_keyboard_output_report;
						event_keyboard_output_report = (EventKeyboardOutputReport *)event;
						if(cm_context->debug_enabled)
							corrib_syslog(LOG_DEBUG,"%s: got a keyboard output report event mask=%u\n",  __func__,event_keyboard_output_report->mask);


						connection_manager_handle_keyboard_output_report(cm_context,event_keyboard_output_report->mask);
						eq_event_free(event);
						break;
					}

					default:
						corrib_syslog(LOG_ERR,"CM:%s: got an unknown event type from the Hardware Manager:%u ",  __func__,event->type);
						eq_show_event_type(event->type);
						eq_event_free(event); //we still need to free the event
						break;
					}

				}
                               #ifdef VERBOSE_DEBUGGING
                               corrib_syslog(LOG_DEBUG,"CM:%s: checked for events from the hardware Manager.\n",  __func__);
                               #endif
			}

			if(((tv.tv_sec == 0) && (tv.tv_usec == 0)) || (last_processed + HW_DEFAULT_INTERVAL_PERIOD < time(NULL)) ) // this will timeout if we have no other events
			{
				known_event = true;
				connection_manager_process_interval_tasks(cm_context);
				last_processed = time(NULL);
			}

			if (known_event == false)
				corrib_syslog(LOG_WARNING,"CM:%s: got an unknown event from select.\n",  __func__);
			/*
			else
			{
				corrib_syslog(LOG_WARNING,"%s: got unexpected event.\n", __func__);
			}
			*/

		}

	} //end of while loop
	close(cm_context->netlink_sock_fd);
	pthread_exit(NULL);
	return NULL;

}



//Connection Management Core
//------------------------------

/*
 * The connection manager has a list of peers, when a connection is closed the manager must remove that peer from the list
 * in the current model the listner creates the freerdp_peer, the context associated with the peer is freed at the end of the peer loop
 * In the new model the connection manager will create the peer and will also take responsibility for freeing the peer.
 * The connection manager needs to know how many existing peers it has and what characteristics they have
 * All peers join in unicast private mode, only once the connection is established do we know if the peer has requested a shared connection
 * once a peer indicates that a shared connection is required the connection manager must be informed that we are in shared mode.
 * It is then responsible for activating the multi-cast peer.
 * The multicast peer must be aware of the current number of active peers and their IP addresses
 * At any point in time in a multicast session the connection manager must be aware of the KM owner
 */

//Connection Peer List Manipulation
//------------------------------------------------------------------
boolean  connection_manager_peer_list_insert(cmContext * cm_context,freerdp_peer* client)
{

	boolean status = true;
    peerNode * node = xmalloc(sizeof(peerNode),__func__);      /* Insert at the head. */
    node->id  = 1;
    node->client = client;
    //first check to see if we have this connection already
    if(connection_manager_find_client_node(cm_context,client))
    {
    	corrib_syslog(LOG_WARNING,"CM:%s: Attempting to add client (%s) that already exists.\n", __func__,client->hostname);
    	status = false;
    	xfree(node,__func__); // this is required
    }
    else
    	LIST_INSERT_HEAD(&(cm_context->peer_list_head), node, entries);
    return status;
}

void connection_manager_clear_peer_list(cmContext * cm_context)
{

    peerNode * node_iterator;
    LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
    {
	    LIST_REMOVE(node_iterator, entries);
	    xfree(node_iterator,__func__);
    }
}

void connection_manager_clear_peer_wait_queue(cmContext * cm_context)
{

    peerWaitNode * node_iterator;
    TAILQ_FOREACH(node_iterator, &(cm_context->peer_wait_queue_head), entries)
    {
	    TAILQ_REMOVE(&(cm_context->peer_wait_queue_head), node_iterator, entries);
	    xfree(node_iterator,__func__);
    }
}

int connect_manager_get_peer_list_size(cmContext * cm_context)
{
    peerNode * node_iterator;
    int count = 0;
    LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
    {
    	count++;
    } //there is an easier way of doing this using sizeof, but for another day
    return count;
}

int connect_manager_get_peer_list_size_and_types(cmContext * cm_context, uint32_t * optimised_peers, uint32_t * lossless_peers)
{
	peerNode * node_iterator;
	int count = 0;
	*optimised_peers = 0;
	*lossless_peers = 0;
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		count++;
		if( MODE_CHECK(node_iterator->client->compression_mode, OPTIMISED) )
		{
			(*optimised_peers)++;
		}
		else if( MODE_CHECK(node_iterator->client->compression_mode, LOSSLESS) )
		{
			(*lossless_peers)++;
		}
	} //there is an easier way of doing this using sizeof, but for another day
	return count;
}

//Given an fd, find the corresponding peer, return it or NULL if not found
static freerdp_peer * connection_manager_get_peer_for_fd(cmContext * cm_context,int fd)
{

    peerNode * node_iterator;
    LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
    {
	    if(node_iterator->client->sockfd == fd)
	    	return node_iterator->client;
    }
    return NULL;
}

static peerNode * connection_manager_find_client_node(cmContext * cm_context, freerdp_peer* client)
{
    CM_PRINT_FUNC();
    peerNode * node_iterator;
    LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
    {
    	if(node_iterator->client == client)
    		return node_iterator;
    }
    return NULL;

}

//Remove a client from the list based on a client reference
static boolean connection_manager_peer_list_remove(cmContext * cm_context, freerdp_peer* client)
{

	boolean status = true;
	peerNode * node;
	node = connection_manager_find_client_node(cm_context, client);
	if(node)
	{
	    LIST_REMOVE(node, entries);
	    xfree(node,__func__); //FIXME, check that this is required
	}
	else
		status = false;
    return status;
}

static void connection_manager_show_peer_list_entry(peerNode * node)
{

	if(node)
	{
		char buffer[255];
		snprintf(buffer,255,"id:%d,host:%s,fd=%d,p=%p\n",node->id,node->client->hostname,node->client->sockfd,node->client);
		corrib_syslog(LOG_DEBUG,"CM:%s: %s",__func__,buffer);
#if 0
		printf("node id: %d\n",node->id);
		printf("hostname: %s\n",node->client->hostname);
		printf("socketfd: %d\n",node->client->sockfd);
		printf("client pointer: %p\n",node->client);
#endif
	}
	else
		printf("node is null\n");

}

#if 1
static void connection_manager_show_peer_list(cmContext * cm_context)
{
    peerNode * node_iterator;
    LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
    {
    	connection_manager_show_peer_list_entry(node_iterator);
    }
}
#endif

//-----------------------------------------------------------------------
//-------------------------Event Handlers -------------------------------
//-----------------------------------------------------------------------

//------------------------------------------------------------------------


/*
	Function to read the discovery file (if it exists and get the Domain key (i.e MAC) if
	if exists.
	NOTE: This functionality is used on Barrow RX, Corrib TX and Corrib RX
		If you change one, you should think about changing all of them ...

 */
static void connection_manager_get_server_domain_key(uint8 *domain_key)
{

	int imac[6];
	unsigned int i;
	unsigned char line[128];
   	unsigned char *ptr1 = NULL;
	FILE *fp;
	int managed = 0;
	int mgr_found = 0;
	int state_found = 0;
	uint8 default_domain[] = {0x11,0x22,0x33,0x44,0x55,0x66};

	fp = fopen("/usr/local/discovery","r");
	if(fp == NULL)
	{
		/* Error Opening File */
		if(errno == ENOENT)
		{
			/* No File exists, so set the default domain_key */
			corrib_syslog(LOG_DEBUG,"%s() - No discovery file exists ...using default domain key\n",__func__);
			corrib_syslog(LOG_INFO,"%s() - No discovery file exists ...using default domain key\n",__func__);

			for(i=0;i<6;i++)
				domain_key[i] = default_domain[i];
            corrib_syslog(LOG_DEBUG,"%s(): %02x:%02x:%02x:%02x:%02x:%02x\n",__func__,
            			domain_key[5],domain_key[4],domain_key[3],domain_key[2],domain_key[1],domain_key[0]);
		}
		else
		{
			corrib_syslog(LOG_DEBUG,"%s() - Can't open discovery file .....\n",__func__);
			corrib_syslog(LOG_INFO,"%s() - Can't open discovery file  ...\n",__func__);

			for(i=0;i<6;i++)
				domain_key[i] = default_domain[i];
		}
	}
	else
	{
		/* File Exists, so read the data ..... */
		while(fgets(line,sizeof line,fp)!=NULL)
   		{
   			ptr1=strstr(line,"MGR=");
			if(ptr1 != NULL)
			{
				mgr_found = 1;
				ptr1 += strlen("MGR="); /* Jump to the actual MAC address..... */
				sscanf(ptr1,"%02x:%02x:%02x:%02x:%02x:%02x",&imac[0],&imac[1],&imac[2],&imac[3],&imac[4],&imac[5]);
			}
   			ptr1=strstr(line,"STATE=");
			if(ptr1 == NULL)
				ptr1=strstr(line,"STaTE=");   /* The format used for Barrow ...*/

			if(ptr1 != NULL)
			{
				state_found = 1;
				ptr1 += strlen("STATE="); /* Jump to the actual value of the STATE */
				if(strncmp(ptr1,"Managed",strlen("Managed")) == 0)
				{
					managed = 1;
				}
				else
				{
					managed = 0;
				}
				break;
			}
		}

   		fclose(fp);
		if(!((mgr_found) && (managed)))
		{
			managed = 0;
			
			corrib_syslog(LOG_INFO,"%s() - This is an unmanaged connection\n",__func__);
		}
		else
		{
			corrib_syslog(LOG_INFO,"%s() - This is a managed connection\n",__func__);
		}
	}
	if(managed)
	{
		for(i=0;i<6;i++)
			domain_key[i] = imac[i];
	}
	else /* Unmanaged, so use default domain */
	{
		for(i=0;i<6;i++)
			domain_key[i] = default_domain[i];
	}
    corrib_syslog(LOG_DEBUG,"%s(): %02x:%02x:%02x:%02x:%02x:%02x\n",__func__,
    			domain_key[5],domain_key[4],domain_key[3],domain_key[2],domain_key[1],domain_key[0]);
}

static void connection_manager_set_connecting_client(cmContext * cm_context, freerdp_peer * client, const char * caller)
{


	if (cm_context->connecting_client)
        {
		corrib_syslog(LOG_ERR, "CM:%s->%s: client %s already connecting, failed to set new connecting client %s\n",
			      caller, __func__, cm_context->connecting_client->hostname, client->hostname);
		return;
	}

	cm_context->connecting_client = client;
}

static void connection_manager_unset_connecting_client(cmContext * cm_context, freerdp_peer * client, const char * caller) {


	/* If we don't currently have a connecting client, there's nothing to do */
	if (!cm_context->connecting_client)
		return;

	if (client != cm_context->connecting_client) {
		corrib_syslog(LOG_INFO, "CM:%s->%s: client %s does not match connecting client %s, ignoring\n",
			      caller, __func__, client->hostname, cm_context->connecting_client->hostname);
		return;
	}

	// Finished handling a new connection, now check if other clients are waiting
	cm_context->connecting_client = NULL;
}

static boolean connection_manager_getActive_extDsktp(cmContext * cm_context, freerdp_peer * client)
{

	peerNode * node_iterator;
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		/* Skip check for leaving peer */
		if (node_iterator->client && (node_iterator->client->sockfd != client->sockfd)) {
			if(node_iterator->client->context->rdp->settings->num_monitors == 2)
				return true;
		}
	}

	return false;
}

static void connection_manager_handle_new_client_request(cmContext * cm_context, int client_socket_fd, const char * hostname)
{
    #ifdef SHARED_MODE_DEBUG
	corrib_syslog(LOG_DEBUG,"CM:%s:\n", __func__);
    #endif
	freerdp_peer* client;
	int current_peers = connect_manager_get_peer_list_size(cm_context);

#ifdef CONNECTION_PROFILING
	char command[255];
	sprintf(command,"/opt/blackbox/time_check.sh SERVER cm_new_peer ");
	system(command);
#endif

	//create the new server_peer/peer
	client = freerdp_peer_new(client_socket_fd,cm_context->peer_cm_queue);
	client->connection_id = cm_context->client_id_counter++;
    #ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG,"CM:%s:operating mode is %d, current_peers=%d\n",  __func__,cm_context->cm_operating_mode,current_peers);
    #endif
	client->peer_type = PRIMARY_PEER;
	IFCALL(cm_context->PeerAccepted, cm_context, client); //this call causes all of the subsequent client initialisation including creation of the server_peer context

	strncpy(client->hostname, hostname, 50);
	cm_context->hm_context->signal_new_connection = true;
	connection_manager_peer_list_insert(cm_context, client);
	cm_context->succesful_logins++;

	strncpy(client->connection_hostname, hostname, 255);
	strncpy(client->connection_username, "demo", 255);

	connection_manager_set_connecting_client(cm_context, client, __func__);

	client->connection_start_time = sh_log_get_mstime();



	statistcs_send_json_active_connection_statistic_object("unknown","unknown",client->connection_mode,client->connection_start_time,
			client->connection_id,client->connection_hostname,0);
	corrib_syslog_es (LOG_INFO,"cm_new_client");
}

//This occurs on both a new connection and on a sync loss and res change
static bool connection_manager_handle_res_change_complete(cmContext * cm_context, int client_socket_fd,
							  int head)
{
	bool peer_video_active = false;
	videoHead_t connection_res;
	freerdp_peer * client = connection_manager_get_peer_for_fd(cm_context, client_socket_fd);

	corrib_syslog_bs (LOG_INFO,"cm_res_change_complete");

	hw_manager_get_connection_resolution(cm_context->hm_context, client->compression_mode, &connection_res, head-1);

	corrib_syslog(LOG_INFO,"%s: Client:%s Compression %s, Res %dx%d %dHz, Server Res %dx%d %dHz\n", __func__,
			client->hostname, compression_type_name[client->compression_mode], client->decoder_width[head-1], client->decoder_height[head-1],
			client->decoder_refresh[head-1], connection_res.width, connection_res.height, connection_res.refresh);

	if ((client->connection_state == PEER_CONNECTION_STATE_RUNNING) &&
	(client->video_state[head-1] == PEER_VIDEO_STATE_RES_CHANGE))
	{
		peer_video_active = true;
		client->video_state[head-1] = PEER_VIDEO_STATE_ACTIVE;
		corrib_syslog(LOG_INFO, "%s: Setting head %u video_state=%s for client %s\n",
				__func__, head, cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
	}
	else if((client->connection_state == PEER_CONNECTION_STATE_RUNNING) &&
	(client->video_state[head-1] == PEER_VIDEO_STATE_ACTIVE))
	{
		peer_video_active = true;
		corrib_syslog(LOG_INFO, "%s: Not processing res-change-complete: client %s, head %u already running\n",
				__func__, client->hostname, head-1);
	}
	else
	{
		corrib_syslog(LOG_INFO, "%s: Not processing res-change-complete: connection_state=%s, head %u video state=%s for client %s\n",
				__func__, cm_get_peer_connection_state_string(client->connection_state), head, 
				cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
	}

	if(!((client->decoder_width[head-1] == connection_res.width)
		&& (client->decoder_height[head-1] == connection_res.height)
		&& (client->decoder_refresh[head-1] == connection_res.refresh))) 
	{
		corrib_syslog(LOG_INFO, "Effective resolution doesn't match, check for missed event\n");
		/* Check in case a new sync-loss event occurred again while the res-change was in progress */
		connection_manager_check_missed_res_events(cm_context, client, head);
		//Dont restart video as this client is incorrect at the moment
		peer_video_active = false;
	}

	corrib_syslog(LOG_DEBUG, "%s: %s peer_video_active is %d\n", __func__, client->hostname, peer_video_active);
	corrib_syslog_es (LOG_INFO,"cm_res_change_complete");

	return peer_video_active;
}

boolean connection_manager_fastpath_tcp_init(cmContext *cm_context, freerdp_peer *client)
{
	boolean audio_enabled = client->settings->AnalogAudio;
	/* If no channels have been allocated so far, mark this as the "primary" channel (for municast) */
	boolean primary_video = !(connection_manager_remaining_municast_channel(cm_context, VIDEO_MODE));
	boolean primary_audio = !(connection_manager_remaining_municast_channel(cm_context, AUDIO_MODE));

	/*
	 * On platforms with with TCP syn/fin handshaking support for TOE fast-path connections,
	 * first open the ports to listen for a new connection from the Decoder before responding.
	 * Note that the TOE can only accept a single connection per listening port, but we assume
	 * that we won't be listening for multiple connections concurrently because the
	 * 'cm_context->connecting_client' point will be set until this after peer connects
	 * and other connection attempts will be placed on a wait queue while that pointer is set.
	 */

	/* Allocate video and audio channels and corresponding TOE CT entries for this client */
	client->video_channel = connection_manager_get_municast_channel(cm_context, VIDEO_MODE);
	if (client->video_channel < 0) {
		corrib_syslog(LOG_ERR, "%s:Failed to allocate video channel for peer:%s\n",
			      __func__, client->hostname);
		return false;
	}
	client->video_slave_cid = connection_manager_video_channel_to_cid(client->video_channel);

	if (audio_enabled) {
		client->audio_channel = connection_manager_get_municast_channel(cm_context, AUDIO_MODE);
		if (client->audio_channel < 0) {
			corrib_syslog(LOG_ERR, "%s:Failed to allocate audio channel for peer:%s\n",
				      __func__, client->hostname);
			connection_manager_channel_set_unused(cm_context, VIDEO_MODE, client->video_channel);
			return false;
		}
		client->audio_slave_cid = connection_manager_audio_channel_to_cid(client->audio_channel);
	}

	if (! client_open_tcp_listening_ports(client, cm_context, true, audio_enabled,
					      primary_video, primary_audio, __func__))
	{
		/* BUG-5370: TOE channel is activated here and We cannot reject 
		 * the connection. Rejection marks the peer as TEMPORARY PEER.
		 * Termination logic won't kick in for a temporay peer causing
		 * the channel to stay active always */
		corrib_syslog(LOG_ERR, "%s:Failed to open TCP listening ports for peer:%s\n",
				__func__, client->hostname);
	}

	connection_manager_channel_set_listening(cm_context, VIDEO_MODE, client->video_channel);
	if (audio_enabled)
		connection_manager_channel_set_listening(cm_context, AUDIO_MODE, client->audio_channel);

	return true;
}

//this is the first time we definitively know what the connection type is
static boolean connection_manager_handle_server_peer_ready(cmContext * cm_context,freerdp_peer * client)
{
	boolean accepted = false;
	corrib_syslog_bs (LOG_INFO,"cm_server_peer_ready");
	su_strlcpy(client->settings->client_loggedin_user,  cm_context->loggedin_user,USERNAME_LENGTH);
	int current_peers = connect_manager_get_peer_list_size(cm_context);

#ifdef TRACE_NEGOTIATION
	connection_manager_show_connection_details(cm_context,client, __func__,"start");
#endif
	if (client->connection_state != PEER_CONNECTION_STATE_INIT) {
		corrib_syslog(LOG_ERR, "%s: Invalid connection_state %d [Expected: PEER_CONNECTION_STATE_INIT] for peer %s\n",
			      __func__, client->connection_state, client->hostname);
		connection_manager_terminate_peer(client);
		return accepted;
	}

	if(connection_manager_state_machine(cm_context,client) == 0) //no errors
	{
		bool sync_loss_h1 = !(client->settings->connection_resolution[FIRST_HEAD].width && client->settings->connection_resolution[FIRST_HEAD].height);
		bool sync_loss_h2 = !(client->settings->connection_resolution[SECOND_HEAD].width && client->settings->connection_resolution[SECOND_HEAD].height);

		accepted = true;
		client->connection_state = PEER_CONNECTION_STATE_ACCEPTED;

		/* If sync-loss is present, updated the video state accordingly */
		if (sync_loss_h1) {
			client->video_state[HEAD_1] = PEER_VIDEO_STATE_SYNC_LOSS;
			corrib_syslog(LOG_INFO, "%s: Setting head %u video_state=%s (SYNC_LOSS) for client %s\n",
				      __func__, 1, cm_get_peer_video_state_string(client->video_state[HEAD_1]), client->hostname);
		}
		if ((client->context->rdp->settings->num_monitors == 2) && sync_loss_h2) {
			client->video_state[HEAD_2] = PEER_VIDEO_STATE_SYNC_LOSS;
			corrib_syslog(LOG_INFO, "%s: Setting head %u video_state=%s (SYNC_LOSS) for client %s\n",
				      __func__, 1, cm_get_peer_video_state_string(client->video_state[HEAD_1]), client->hostname);
		}
	}

	corrib_syslog_es (LOG_INFO,"cm_server_peer_ready");
	return accepted;
}

//This occurs on both a new connection and on a sync loss and res change
static void connection_manager_handle_connection_info(cmContext * cm_context, EventConnectionInfo * client_connection_info)
{
#if 1

	corrib_syslog_bs (LOG_INFO,"cm_connection_info");

	int connection_type = client_connection_info->connection_type;
	int client_socket_fd = client_connection_info->client_id;
	freerdp_peer * client = connection_manager_get_peer_for_fd(cm_context,client_socket_fd);
#ifdef TRACE_NEGOTIATION
	connection_manager_show_connection_details(cm_context,client, __func__,"start");
#endif
	char *hostname = client->connection_hostname;
	int current_peers = connect_manager_get_peer_list_size(cm_context);
	boolean extended_desktop = (client->context->rdp->settings->num_monitors == 2);

	if (client->connection_state != PEER_CONNECTION_STATE_ACCEPTED) {
		corrib_syslog(LOG_ERR, "%s: Invalid connection_state %d [Expected: PEER_CONNECTION_STATE_ACCEPTED] for peer %s\n",
			      __func__, client->connection_state, client->hostname);
		connection_manager_terminate_peer(client);
		return;
	}

	if (!MODE_CHECK(connection_type, MULTICAST_MODE))
	{
		if (connection_manager_channel_is_listening(cm_context, VIDEO_MODE, client->video_channel))
		{
			if (!cm_context->video_municast_running)
			{
				/* First Video connection */
				if (!client_start_encoder_video(client, cm_context, client->settings->AnalogAudio,extended_desktop, __func__))
				{
					corrib_syslog(LOG_ERR, "%s:Failed to start EncoderStart Peer:%s\n",__func__, hostname);
					connection_manager_terminate_peer(client);
					return;
				}
			}
			else
			{
				/* Shared Municast Video Connection */
				if (!client_start_encoder_video_mu(client, cm_context, false, extended_desktop, __func__)) {
					corrib_syslog(LOG_ERR, "%s:Failed to start EncoderStart MU Peer:%s\n",__func__, hostname);
					connection_manager_terminate_peer(client);
					return;
				}
			}
			connection_manager_channel_set_active(cm_context, VIDEO_MODE, client->video_channel);
			client->video_state[HEAD_1] = PEER_VIDEO_STATE_ACTIVE;
			if (extended_desktop) client->video_state[HEAD_2] = PEER_VIDEO_STATE_ACTIVE;
		}
		else
		{
			/*
			 * Video channel may not be in listening state if sync-loss was present when connection accepted.
			 * If a res-change occurred since then, it will be handled by deferred sync-loss/res-change
			 * processing after the connection has been established.
			 */
			corrib_syslog(LOG_INFO, "%s: Not starting video channel %d in state %d for client %s\n",
				      __func__, client->video_channel, cm_context->video_channels[client->video_channel], client->hostname);
		}

		if (client->settings->AnalogAudio)
		{
			if (connection_manager_channel_is_listening(cm_context, AUDIO_MODE, client->audio_channel))
			{
				if (!cm_context->audio_municast_running)
				{
					/* First Audio connection */
					if (!client_start_encoder_audio(client, cm_context, (MODE_CHECK(connection_type, MUNICAST_MODE )) ? true : false, __func__))
					{
						corrib_syslog(LOG_ERR, "%s:Failed to start EncoderStart Peer:%s\n",__func__, hostname);
						connection_manager_terminate_peer(client);
						return;
					}
				}
				else
				{
					/* Shared Municast Audio connection */
					if (!client_start_encoder_audio_mu(client, __func__))
					{
						corrib_syslog(LOG_ERR, "%s:Failed to start AudioStart Peer:%s\n",__func__, hostname);
						connection_manager_terminate_peer(client);
						return;
					}
				}
				connection_manager_channel_set_active(cm_context, AUDIO_MODE, client->audio_channel);
			}
			else
			{
				/* Audio channel may not be in listening state if not requested by client */
                #ifdef DEBUG_ENABLED
				corrib_syslog(LOG_DEBUG, "%s: Not starting audio channel %d in state %d\n",
					      __func__, client->audio_channel, cm_context->audio_channels[client->audio_channel]);
                #endif
			}
		}
	}
	else //multicast requested
	{
		/* Multicast Master Start */
		if(cm_context->video_multicast_running == false)
		{
			/* Start video master */
			if(client_master_start(cm_context, client))
				corrib_syslog(LOG_INFO,"%s:Shared mode EncoderMasterStart Peer:%s\n",__func__, hostname);
			else
				corrib_syslog(LOG_ERR,"%s:Failed to start EncoderMasterStart Peer:%s\n",__func__, hostname);
		}
		else
			corrib_syslog(LOG_INFO,"%s:EncoderMaster is already running in shared mode\n",__func__);

		/* Multicast Slave Start */
		if(client_slave_start_video(client))
			corrib_syslog(LOG_INFO,"%s:Shared mode EncoderSlaveStartVideo Peer:%s\n",__func__,hostname);
		else
			corrib_syslog(LOG_ERR,"%s: Failed to start EncoderSlaveStartVideo Peer:%s\n",__func__,hostname);

		/* Multicast Master resume */
		if(client_master_resume_video(cm_context))
			corrib_syslog(LOG_INFO,"%s:Shared mode EncoderMasterResume Peer:%s\n",__func__, hostname);
		else
			corrib_syslog(LOG_ERR,"%s:Failed to start EncoderMasterResume Peer:%s\n",__func__, hostname);

		client->video_state[HEAD_1] = PEER_VIDEO_STATE_ACTIVE;

		if (client->settings->AnalogAudio) {
			if (cm_context->audio_multicast_running == false) {
				cm_context->audio_master_mu_channel = connection_manager_get_municast_channel(cm_context, AUDIO_MODE);
				/* Start audio master */
				client_audio_master_start(cm_context, client);
			}

			if(client_slave_start_audio(client))
				corrib_syslog(LOG_INFO,"%s:Shared mode EncoderSlaveStartAudio Peer:%s\n",__func__,hostname);
			else
				corrib_syslog(LOG_ERR,"%s: Failed to start EncoderSlaveStartAudio Peer:%s\n",__func__,hostname);

			client_master_resume_audio();
		}

#ifdef SHARED_MODE_DEBUG
		connection_manager_show_slave_cid_pool(cm_context);
#endif

	}

	/* BUG-4525: In a certain race condition a leaving connection could reset
	 * the server mode while a new connection is still initializing. 
	 * A this point we are sure that the new connection is matured and
	 * we should set the compression mode here */
	connection_manager_set_server_mode(cm_context, client);

	client->connection_state = PEER_CONNECTION_STATE_STARTING;
	corrib_syslog(LOG_INFO, "%s: Setting connection_state=%d (STARTING) for client %s\n",
		      __func__, client->connection_state, client->hostname);

	corrib_syslog_es (LOG_INFO,"cm_connection_info");


#endif
}

static void connection_manager_check_sync_loss(cmContext *cm_context, freerdp_peer *client, int head)
{
	videoHead_t input_res = cm_context->hm_context->ingress_resolution[head-1];
	boolean sync_loss = (!input_res.width && !input_res.height);

	if (sync_loss && (client->video_state[head-1] == PEER_VIDEO_STATE_ACTIVE))
	{
		corrib_syslog(LOG_INFO,"%s: Video not present for head %d, initiate sync-loss ...\n",
				__func__, head);
		connection_manager_handle_sync_loss(cm_context, head);
	}
}

static void connection_manager_check_missed_res_events(cmContext *cm_context, freerdp_peer *client, int head)
{
	videoData_t connectionResData;
	connectionResData.ingress_resolution = cm_context->hm_context->ingress_resolution[head-1];
	connectionResData.sync_loss = (!connectionResData.ingress_resolution.width && !connectionResData.ingress_resolution.height);
	connectionResData.optimised_egress_res = cm_context->hm_context->optimised_egress_res[head-1];
	if( FIRST_HEAD == (head - 1) )
	{
		connectionResData.lossless_egress_res = cm_context->hm_context->lossless_egress_res;
	}
	else
	{
		connectionResData.lossless_egress_res = (videoHead_t){0, 0, 0};
	}
	
	/* Deferred Sync loss */
	if (connectionResData.sync_loss && (client->video_state[head-1] == PEER_VIDEO_STATE_ACTIVE))
	{
		corrib_syslog(LOG_INFO,"%s: processing deferred sync-loss event for head %u\n",
			      __func__, head);
		connection_manager_handle_sync_loss(cm_context, head);
	}
	/* Deferred Res change */
	else if (!connectionResData.sync_loss && (client->video_state[head-1] == PEER_VIDEO_STATE_SYNC_LOSS))
	{
		corrib_syslog(LOG_INFO,"%s: processing deferred res-change event for head %u\n",
			      __func__, head);
		connection_manager_handle_resolution_change(cm_context, head, connectionResData);
	}
	/* We could miss Sync loss-Res change pair due to back to back res event (BUG-3406)
	 * If decoder is setup on resolution other then encoder's detected res 
	 * process both syncloss and res change again */
	else if (!connectionResData.sync_loss && (client->video_state[head-1] == PEER_VIDEO_STATE_ACTIVE))
	{
		corrib_syslog(LOG_INFO,"%s: processing missed sync-loss and res-change event for head %u\n",
				__func__, head);
		connection_manager_handle_sync_loss(cm_context, head);
		connection_manager_handle_resolution_change(cm_context, head, connectionResData);
	}
}

static void connection_manager_handle_client_ready(cmContext * cm_context,int client_socket_fd,int connection_mode,
		boolean preemption_requested,uint8 * domain_key,uint32 session_id,char * loggedin_user)
{

	corrib_syslog_bs (LOG_INFO,"cm_client_ready");
	int video_remote_port = 0, audio_remote_port = 0;
	int timeout = 0;

	freerdp_peer * client = connection_manager_get_peer_for_fd(cm_context,client_socket_fd);
#ifdef TRACE_NEGOTIATION
	connection_manager_show_connection_details(cm_context,client, __func__,"Start");
#endif
	if (client->connection_state != PEER_CONNECTION_STATE_STARTING) {
		corrib_syslog(LOG_ERR, "%s: Invalid connection_state %d [expected: PEER_CONNECTION_STATE_STARTING ] for peer %s\n",
			      __func__, client->connection_state, client->hostname);
		connection_manager_terminate_peer(client);
		return;
	}

	client->client_ready_count++;
	int remaining_peers = connect_manager_get_peer_list_size(cm_context);
	corrib_syslog(LOG_INFO,"CM:%s: multicast_status %u SESSION-ID=%d Remaining Peers = %d\n", __func__,connection_mode,session_id,remaining_peers);
	client_show_compression_mode(client->compression_mode,__func__);
	char client_domain_key_string[MAX_ALERT_CONTEXT_SIZE];
    snprintf(client_domain_key_string,MAX_ALERT_CONTEXT_SIZE,"%x:%x:%x:%x:%x:%x",domain_key[0],domain_key[1],
        domain_key[2],domain_key[3],domain_key[4],domain_key[5]);

	statistcs_send_json_alert_object(INFO,
			OPERATION,CONNECTION,
			START,
			SUCCESS,"New Connection",client_domain_key_string,client->hostname,"","");
			corrib_syslog(LOG_DEBUG,"%s copying %s.....\n",__func__,loggedin_user);
	        su_strlcpy(cm_context->loggedin_user,loggedin_user, USERNAME_LENGTH);
	if(client != NULL && (remaining_peers > 0))
	{
		client->settings->width = client->settings->connection_resolution[FIRST_HEAD].width;
		client->settings->height = client->settings->connection_resolution[FIRST_HEAD].height;
	}

	/* BUG-5370: Before servicing waiting clients check TCP states of current client 
	 * We are checking remote ports and a non-zero port indicates TCP link
	 * up event */
	do {
		su_toe_get_remote_port(client->video_slave_cid, &video_remote_port);
		if (client->settings->AnalogAudio)
			su_toe_get_remote_port(client->audio_slave_cid, &audio_remote_port);

		/* Check for Audio remote port only if Audio is enabled in a
		 * connection */
		if ((video_remote_port && !(!(client->settings->AnalogAudio) != !(audio_remote_port))) != 0) {
			corrib_syslog(LOG_DEBUG, "Connection Established\n");
			break;
		}
		usleep(100000);
	} while( ++timeout < 10 );

	client->connection_state = PEER_CONNECTION_STATE_RUNNING;

	//if new connection is exclusive user, provide full K/M arbitration to the exclusive client
	if(MODE_CHECK(connection_mode, EXCLUSIVE_MODE))
	{
		int exclusive_request = true;
		connection_manager_arbitrate_keyboard_mouse_control(cm_context,client_socket_fd, exclusive_request);
	}

	corrib_syslog(LOG_INFO, "%s: Setting connection_state=%d (RUNNING) for client %s\n",
		      __func__, client->connection_state, client->hostname);
	connection_manager_unset_connecting_client(cm_context, client, __func__);

#ifdef ENABLE_WEB_SERVICE_REPORTING
	statistcs_send_json_authentication_statistic_object(cm_context->succesful_logins,cm_context->failed_logins,0);
#endif

	connection_manager_check_sync_loss(cm_context, client, 1);
	if (client->context->rdp->settings->num_monitors == 2)
	{
		connection_manager_check_sync_loss(cm_context, client, 2);
	}

	/* Check if any other new clients are waiting to connect */
	connection_manager_process_wait_queue(cm_context);

	corrib_syslog_es (LOG_INFO,"cm_client_ready");
}

/*
 * A peer has been terminated 
 */
static void connection_manager_handle_peer_termination(cmContext * cm_context, int client_socket_fd)
{
	corrib_syslog_bs (LOG_INFO,"cm_peer_termination");
	uint32 client_id;
	char command[255];
	freerdp_peer * client = connection_manager_get_peer_for_fd(cm_context,client_socket_fd);
	boolean reset_hardware = false;

#ifdef TRACE_NEGOTIATION
	connection_manager_show_connection_details(cm_context,client, __func__,"start");
#endif
	if(client)
	{
		client->terminating = true;
		client_id = client->connection_id;
		eq_clear_events(client->cm_peer_queue);
		connection_manager_utiles_show_client_info(client, true, __func__);
	}
	else
	{
#ifdef TRACE_NEGOTIATION
		corrib_syslog(LOG_DEBUG,"CON-DETAIL: %s:Could not find the client for termination\n",__func__);
#endif
		corrib_syslog(LOG_INFO,"CM:%s:Could not find the client for termination\n", __func__);
		return;
	}

	int remaining_peers = connect_manager_get_peer_list_size(cm_context);
	if(client != NULL && (remaining_peers > 0))
	{
		client->connection_end_time = sh_log_get_mstime();
		if (client->peer_type != TEMPORARY_PEER) {
			/* If server running Multicast and peer is Multicast */
			if (( (MODE_CHECK(cm_context->cm_operating_mode, MULTICAST_MODE)))
					&& ((MODE_CHECK(client->connection_mode, MULTICAST_MODE))))
			{
				/* Terminate Video Slave */
				if(connection_manager_replenish_slave_cid_pool(cm_context, client, VIDEO_MODE))
				{
					client_slave_stop_video(client);
				}
				else
				{
					corrib_syslog(LOG_ERR,"%s: Failed to find Video Slave CID[%02x] for %s.\n",
							__func__, client->video_slave_cid, client->hostname);
				}

				// if the cm_operating_mode is exclusive, change to multicast mode
				if(MODE_CHECK(cm_context->cm_operating_mode, EXCLUSIVE_MODE) && cm_context->controlling_peer_id == client_socket_fd)
				{
					MODE_REMOVE(cm_context->cm_operating_mode, EXCLUSIVE_MODE);
				}
				/* Terminate Video Master, if no video slave left */
				if (!connection_manager_check_remaining_video_slave(cm_context)) {
					corrib_syslog(LOG_INFO,"%s: All video channels stopped, terminating video master.\n",  __func__);
					client_master_stop(cm_context);
					client_tear_down_slaves(VIDEO_MODE);
					MODE_REMOVE(cm_context->cm_operating_mode, MULTICAST_MODE);
					MODE_REMOVE(cm_context->cm_compression_mode, LOSSLESS);
					cm_context->hm_context->configured_compression = cm_context->cm_compression_mode;
				}

				/* Terminate Audio slave and master if no slave left */
				if(client->settings->AnalogAudio) {
					/* Terminate Audio Slave */
					if(connection_manager_replenish_slave_cid_pool(cm_context, client, AUDIO_MODE))
					{
						client_slave_stop_audio(client);
					}
					else
					{
						corrib_syslog(LOG_ERR,"%s: Failed to find Audio slave CID[%02x] for %s.\n",
								__func__, client->audio_slave_cid, client->hostname);
					}

					/* Stop audio master if no audio peer left in system */
					if(!connection_manager_check_remaining_audio_slave(cm_context)) {
						corrib_syslog(LOG_INFO,"%s: All audio channels stopped, terminating audio master.\n",  __func__);
						connection_manager_channel_set_unused(cm_context, AUDIO_MODE, cm_context->audio_master_mu_channel);
						client_audio_master_stop(cm_context);
						client_tear_down_slaves(AUDIO_MODE);
					}
				}
#ifdef SHARED_MODE_DEBUG
				connection_manager_show_slave_cid_pool(cm_context);
				corrib_syslog(LOG_INFO,"%s: Setting terminating to true for %s.\n",__func__,client->hostname);
#endif

#ifdef TRACE_NEGOTIATION
				connection_manager_show_connection_details(cm_context,client, __func__,"IDLE POINT");
#endif

			/* For all non-multicast clients */
			} else if (MODE_CHECK(cm_context->cm_operating_mode, UNICAST_MODE | MUNICAST_MODE)
					&& MODE_CHECK(client->connection_mode, UNICAST_MODE | MUNICAST_MODE)) {
#ifdef DEBUG_ENABLED
				corrib_syslog(LOG_DEBUG,"cm_compression_mode is %u\n",cm_context->cm_compression_mode);
#endif

				// if the cm_operating_mode is municast exclusive, change to municast mode
				if(MODE_CHECK(cm_context->cm_operating_mode, EXCLUSIVE_MODE) && cm_context->controlling_peer_id == client_socket_fd)
				{
					MODE_REMOVE(cm_context->cm_operating_mode, EXCLUSIVE_MODE);
				}
				if (connection_manager_channel_is_listening(cm_context, VIDEO_MODE, client->video_channel) ||
				     connection_manager_channel_is_active(cm_context, VIDEO_MODE, client->video_channel)) {
					boolean stop_ext_dsktp = false;
					boolean video_enabled = connection_manager_channel_is_active(cm_context, VIDEO_MODE, client->video_channel);
					unsigned active_channels = connection_manager_active_municast_channels(cm_context, VIDEO_MODE);
					unsigned listening_channels = connection_manager_listening_municast_channels(cm_context, VIDEO_MODE);

					boolean stop_capture = (video_enabled && (active_channels == 1)); // Last active video channel

					/* If a DH peer is leaving, check for other DH in session 
					 * except for the last peer. if no more
					 * DH left in the session turn off H1 */
					if (client->context->rdp->settings->num_monitors == 2
							&& (active_channels > 1)
							&& !(connection_manager_getActive_extDsktp(cm_context, client)))
					{
						stop_ext_dsktp = true;
					}
					
					client_stop_encoder_video(client, cm_context, active_channels, stop_ext_dsktp, stop_capture, __func__);

					/* Reset 2K capture block when last
					 * optimize peer left */
					if (stop_capture) {
						if (client->compression_mode == OPTIMISED) {
							hw_manager_reset_fpga_logic(cm_context->hm_context);
						}
					}

					/* BUG-5652: Reset the mode when no
					 * active or listening channel left */
					if (active_channels + listening_channels == 1) {
						MODE_REMOVE(cm_context->cm_operating_mode, client->connection_mode);
						MODE_REMOVE(cm_context->cm_compression_mode, client->compression_mode);
						cm_context->hm_context->configured_compression = cm_context->cm_compression_mode;
					}
				}
				connection_manager_channel_set_unused(cm_context, VIDEO_MODE, client->video_channel);

				if (client->settings->AnalogAudio) {
					if (connection_manager_channel_is_listening(cm_context, AUDIO_MODE, client->audio_channel) ||
					    connection_manager_channel_is_active(cm_context, AUDIO_MODE, client->audio_channel)) {
						boolean audio_enabled = connection_manager_channel_is_active(cm_context, AUDIO_MODE, client->audio_channel);
						unsigned active_channels = connection_manager_active_municast_channels(cm_context, AUDIO_MODE);
						boolean stop_capture = (audio_enabled && (active_channels == 1)); // Last active audio channel
						client_stop_encoder_audio(client, cm_context, active_channels, stop_capture, __func__);
					}
					connection_manager_channel_set_unused(cm_context, AUDIO_MODE, client->audio_channel);
				}
			}
#ifdef ENABLE_WEB_SERVICE_REPORTING
			statistcs_send_json_control_object("transform_active_previous"); //we only want to transfer non auxilary connections
#endif
			statistcs_send_json_control_object("transform_active_previous"); //we only want to transfer non auxilary connections
		}

		if(connection_manager_peer_list_remove(cm_context,client))
		{
			remaining_peers = connect_manager_get_peer_list_size(cm_context);
#ifdef DEBUG_ENABLED
			corrib_syslog(LOG_INFO,"%s: Removed peer id:%d from peer list. %d peers remaining\n",  __func__,client_socket_fd,remaining_peers);
#endif

#ifdef ENABLE_WEB_SERVICE_REPORTING
			connection_manager_refresh_active_connection_stats(cm_context);
#endif

			statistcs_send_json_alert_object(INFO,OPERATION,CONNECTION,DISCONNECT,SUCCESS,"Connection Termination",client->hostname,"","","");
#ifdef TRACE_NEGOTIATION
			connection_manager_show_connection_details(cm_context,client, __func__,"end");
#endif
			client->Disconnect(client);
			close(client->sockfd);
			client->sockfd = 0;

			connection_manager_unset_connecting_client(cm_context, client, __func__);

			freerdp_peer_context_free(client);
			freerdp_peer_free(client);

			if(remaining_peers == 0)
			{
				corrib_syslog(LOG_INFO, "%s: No active video channels remaining, flushing h/w events and resetting FPGA",  __func__);
				hw_manager_set_media_suspend_state(cm_context->hm_context, AUDIO, true);
				if(cm_context->hm_context->hm_cm_queue->count > 0)
				{
					hw_manager_cleanup(cm_context->hm_context);
				}
				cm_context->cm_operating_mode = UNKNOWN_CONNECTION_MODE;
				cm_context->cm_compression_mode = UNKNOWN_COMPRESSION;
				cm_context->hm_context->configured_compression = UNKNOWN_COMPRESSION;
			}
		}
		else
		{
			corrib_syslog(LOG_ERR,"%s: Failed to remove peer id:%d from peer list.\n",  __func__,client_socket_fd);
#ifdef TRACE_NEGOTIATION
			corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u  %s: Failed to remove peer from peer list.\n",client_id,__func__);
#endif
		}
	}
	else
	{
		corrib_syslog(LOG_ERR,"%s: Request to remove peer id:%d, which does not exist.\n",  __func__,client_socket_fd);
#ifdef TRACE_NEGOTIATION
		corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u  %s: Request to remove peer which does not exist.\n",client_id,__func__);
#endif
	}

	/*
	* ARPM: Jira EM-691
	*/
	if(cm_context->controlling_peer_id == client_socket_fd)
	{
		eq_purge(EQ_EVENT_MOUSE, cm_context->cm_hm_queue);
		eq_purge(EQ_EVENT_KEYBOARD, cm_context->cm_hm_queue);
		hw_manager_flush_pressed_keys(cm_context->hm_context);
		cm_context->hm_context->keys_held = 0;
	}

	/* Check if any other new clients are waiting to connect */
	connection_manager_process_wait_queue(cm_context);

#ifdef TRACE_NEGOTIATION
	corrib_syslog(LOG_DEBUG,"CON-DETAIL:client Client_ID=%u  %s: Completed\n",client_id,__func__);
#endif
	corrib_syslog_es (LOG_INFO,"cm_peer_termination");
}

/* Function: 	Handle resolution change event
 * Arguments: 	1). Pointer to connection manager context
 * 		2). Head ID
 */
static void connection_manager_handle_resolution_change(cmContext * cm_context, int head, const videoData_t connectionResData)
{
	corrib_syslog_bs (LOG_INFO,"cm_res_change");
	peerNode * node_iterator;
	freerdp_peer *client;

	corrib_syslog(LOG_INFO, "%s: Head=%d, sync_loss=%d\n", __func__, head, connectionResData.sync_loss);

	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if(node_iterator)
		{
			client = node_iterator->client;
			if ( connection_manager_handle_resolution_change_for_client(client, head, connectionResData) )
			{
				cm_context->resolution_change_needed[head-1] = true;
			}
		}
		else
		{
			if(cm_context->debug_enabled)
				corrib_syslog(LOG_INFO,"%s: Not processing command because we have no connections\n", __func__);
		}
	}
	corrib_syslog_es (LOG_INFO,"cm_res_change");
}

static boolean connection_manager_handle_resolution_change_for_client(freerdp_peer * client, int head, const videoData_t connectionResData)
{
	boolean resolution_change_needed = false;
	boolean greater_than_hd = false;

	/* Don't send res-change event to extended desktop on a
	SH client, num_monitor = 0 = SH */
	if (head == 2 && client->settings->num_monitors == 0)
		return resolution_change_needed;

	if (client->update)
	{
		client->settings->unscaled_input_resolution[head-1] = connectionResData.ingress_resolution;
		if( LOSSLESS == client->compression_mode)
		{
			client->settings->connection_resolution[head-1] = connectionResData.lossless_egress_res;
		}
		else if(OPTIMISED == client->compression_mode)
		{
			client->settings->connection_resolution[head-1] = connectionResData.optimised_egress_res;
			greater_than_hd = connection_manager_utils_res_above_HD(&connectionResData.optimised_egress_res);
			if(((client->settings->client_technology_type != EMERALD_RA_CLIENT)
				&& (client->settings->client_technology_type != EMERALD_DV_CLIENT)
				&& (client->settings->client_technology_type !=  EMERALD_4K_CLIENT)
			   )
			   && (greater_than_hd))
			{
				corrib_syslog(LOG_INFO, "%s:Terminating client %s, Cloudlinc resolution is greater then HD (1920x1200). \ 
						Connection_state=%s, head %d video_state=%s\n", __func__, client->hostname,
						cm_get_peer_connection_state_string(client->connection_state), head,
						cm_get_peer_video_state_string(client->video_state[head-1]));

				bb_connection_send_reject_connection(client, OPTIMISED_HD_RESOLUTION_EXCEEDED_ERROR_CODE);
				return false;
			}
		}
		//MD TODO - What is the story with the below values
		client->settings->width = client->settings->connection_resolution[FIRST_HEAD].width;
		client->settings->height = client->settings->connection_resolution[FIRST_HEAD].height;

		if ((client->connection_state == PEER_CONNECTION_STATE_RUNNING) &&
				(client->video_state[head-1] == PEER_VIDEO_STATE_SYNC_LOSS) &&
				(connectionResData.sync_loss == false))
		{
			bb_connection_send_resolution_change(client, head);
			client->video_state[head-1] = PEER_VIDEO_STATE_RES_CHANGE;
			resolution_change_needed = true;

			corrib_syslog(LOG_INFO, "%s: Setting head %d video_state=%s (RES_CHANGE) for client %s\n",
					__func__, head, cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
		}
		else
		{
			corrib_syslog(LOG_INFO, "%s: Not sending res-change: connection_state=%s, head %d video_state=%s for client %s\n",
					__func__, cm_get_peer_connection_state_string(client->connection_state), head, cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
		}
	}
	return resolution_change_needed;
}

void connection_manager_get_client_ipaddress(cmContext * cm_context,freerdp_peer* client,char ipaddress[])
{

	peerNode * node = NULL;
	if(client)
		node = connection_manager_find_client_node(cm_context,client);
	else
		node = LIST_FIRST(&(cm_context->peer_list_head));

	if(node)
	{
		strncpy(ipaddress,node->client->hostname,50);
	}
	else
		strncpy(ipaddress,"Not Found",50);
}

void connection_manager_handle_keyboard_output_report(cmContext * cm_context,uint8 mask)
{

	corrib_syslog_bs (LOG_INFO,"cm_keyboard_output");
	//each active connection must be informed of this event
	//if we have no connections then we must store the mask and send it on each new connection
	if(!LIST_EMPTY(&cm_context->peer_list_head))
	{
		peerNode * node_iterator;
		LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
		{

			node_iterator->client->settings->outputReportBitmask = mask;
			node_iterator->client->settings->outputReportAvailable = true;
		}

	}

	corrib_syslog_es (LOG_INFO,"cm_keyboard_output");
}

/* Function: 	Handle sync loss event
 * Arguments: 	1). Pointer to connection manager context
 * 		2). Head ID
 */
static void connection_manager_handle_sync_loss(cmContext * cm_context, int head)
{
	corrib_syslog_bs (LOG_INFO,"cm_sync_loss");

	peerNode * node_iterator = NULL;

	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries) 
	{
		if(node_iterator) 
		{
			freerdp_peer * client = node_iterator->client;
			connection_manager_handle_sync_loss_for_client(client, head);
		}
	}
	corrib_syslog_es (LOG_INFO,"cm_sync_loss");
}

static void connection_manager_handle_sync_loss_for_client(freerdp_peer * client, int head)
{
	/* Don't send syncloss event to extended desktop on a
	SH client, num_monitor=0= SH */
	if (head == 2 && client->settings->num_monitors == 0)
		return;
	
	corrib_syslog(LOG_INFO,"%s: Processing sync loss for %s on head %u.\n", __func__, client->hostname, head);

	client->settings->connection_resolution[head-1] = (videoHead_t){.width = 0, .height = 0, .refresh = 0};
	client->settings->width = 0;
	client->settings->height = 0;

	if(client->update)
	{
		if ((client->connection_state == PEER_CONNECTION_STATE_RUNNING) &&
				(client->video_state[head-1] == PEER_VIDEO_STATE_ACTIVE))
		{
			bb_connection_send_sync_loss(client, head);
			client->video_state[head-1] = PEER_VIDEO_STATE_SYNC_LOSS;

			corrib_syslog(LOG_INFO, "%s: Setting head %u video_state=%s (SYNC_LOSS) for client %s\n",
					__func__, head, cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
		}
		else
		{
			corrib_syslog(LOG_INFO, "%s: Not sending sync-loss: connection state=%s, head %u video_state=%s for client %s\n",
					__func__, cm_get_peer_connection_state_string(client->connection_state), head, cm_get_peer_video_state_string(client->video_state[head-1]), client->hostname);
		}
	}
}


static void connection_manager_show_connection_details(cmContext * cm_context,freerdp_peer * client, const char * func,const char * comment)
{
	char * modes[4]={" UNICAST "," MULTICAST "," MUNICAST ","UNKNOWN"};
	char * peer_types[3]={"PRIMARY_PEER", "TEMPORARY_PEER"};
	int current_peers = connect_manager_get_peer_list_size(cm_context);
	corrib_syslog(LOG_DEBUG,"****\nCON-DETAIL:client Client_ID:%u %s: connection type:%s[%u]\n"
			"Operating mode:%s[%u], current peers:%u,Peer_type:%s\n"
			"Slave CID:%u, vid-seq:%u IP:%s %s\n*****\n",
			client->connection_id,func,
			modes[client->connection_mode],client->connection_mode,
			modes[cm_context->cm_operating_mode],cm_context->cm_operating_mode,
			current_peers, peer_types[client->peer_type],
			client->video_slave_cid,client->video_sequence_number,client->hostname,comment);


}


static void connection_manager_handle_fault_report(cmContext * cm_context)
{
	corrib_syslog_bs (LOG_INFO,"fault_report");
	int remaining_peers = connect_manager_get_peer_list_size(cm_context);
	if(remaining_peers > 0)
	{
    	peerNode * node_iterator;

		LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
		{

			if(node_iterator->client && node_iterator->client->client_ready) //only send video when the client has completed all negotiation
			{
				corrib_syslog(LOG_INFO,"CM:sending error_info_to client %s: ERRINFO_FPGA_RESET_ERROR\n",node_iterator->client->hostname);
				EventReportFault * fault_report_event =  event_report_fault_new(ERRINFO_FPGA_RESET_ERROR,"","","","");
				eq_push(node_iterator->client->cm_peer_queue, (eqEvent *)fault_report_event);
			}
			else if(node_iterator->client->client_ready == false)
			{
				corrib_syslog(LOG_DEBUG,"CM:failed sending error_info_to client %s, client not ready\n",node_iterator->client->hostname);
			}
		}


	}
	sleep(2);
	corrib_syslog(LOG_ERR,"%s: FPGA Failure, rebooting the unit\n", __func__);
	corrib_syslog_es (LOG_INFO,"fault_report");
	//system("echo 'would reboot now'");
	system("reboot");
	exit(0);

}


boolean connection_manager_all_clients_ready(cmContext * cm_context)
{

	peerNode * node_iterator;
	boolean ready =true;
	corrib_syslog_bs (LOG_INFO,"all_clients_ready");
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
#ifdef SHARED_MODE_DEBUG
		corrib_syslog(LOG_INFO,"%s: Checking readiness for %s ready=%d.\n",__func__,node_iterator->client->hostname,node_iterator->client->client_ready);
#endif
		if(node_iterator->client && (!node_iterator->client->client_ready)) //only send video when the client has completed all negotiation
		{

			ready = false;
			break;
		}
	}
	corrib_syslog_es (LOG_INFO,"all_clients_ready");
	return ready;
}


/*
 * We need to find the currently connected peer (which could be a multicast peer) and hand this command off to the peer
 * The peer is responsible for delivering the sending the command
 */
static void connection_manager_handle_audio_command(cmContext * cm_context,EventAudioCommandAvailable * audio_command_available_event)
{

	//corrib_syslog(LOG_INFO,"%s: for head %d.\n", __func__,head);
	//this will eventually be more elaborate but .....
	//what mode are we in
	//if shared mode then give this to the multicast peer
	//if private give it to the current connection which is the head of the list
	//if multi unicast iterate the list and send
	peerNode * node = LIST_FIRST(&(cm_context->peer_list_head));
	if(node)
	{
		rdpUpdate* update = node->client->update;
    	if(update)
    	{
    		pthread_mutex_lock(&(cm_context->mutex));
    		eq_push(node->client->cm_peer_queue, (eqEvent *)audio_command_available_event);
    		pthread_mutex_unlock(&(cm_context->mutex));
    	}
	}


}

/*
 * We need to find the currently connected peer (which could be a multicast peer) and hand this command off to the peer
 * The peer is responsible for delivering the sending the command
 */
static void connection_manager_handle_usb_command(cmContext * cm_context, EventUsbCommandAvailable * usb_command_available_event)
{
#ifdef USBR_SEQUENCE_TRACING
	corrib_syslog_bs (LOG_DEBUG,"cm_usb_command");
#endif
	//corrib_syslog(LOG_INFO,"%s: for head %d.\n", __func__,head);
	//this will eventually be more elaborate but .....
	//what mode are we in
	//if shared mode then give this to the multicast peer
	//if private give it to the current connection which is the head of the list
	//if multi unicast iterate the list and send
	peerNode * node = LIST_FIRST(&(cm_context->peer_list_head));
	if(node)
	{
		rdpUpdate* update = node->client->update;

		if(update)
		{
			pthread_mutex_lock(&(cm_context->mutex));
			eq_push(node->client->cm_peer_queue, (eqEvent *)usb_command_available_event);

			pthread_mutex_unlock(&(cm_context->mutex));
		}
		else
		{
			usb_command_available_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
			eq_push(cm_context->cm_hm_queue,(eqEvent *)usb_command_available_event);
		}
	}
	else
	{

		usb_command_available_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
		eq_push(cm_context->cm_hm_queue,(eqEvent *)usb_command_available_event);
	}
#ifdef USBR_SEQUENCE_TRACING
	corrib_syslog_es (LOG_DEBUG,"cm_usb_command");
#endif
}


/*!
 * @brief Sets the resolution values in the rdpSettings structure for the required head
 *		  handles the logic for determining if scaling is needed also
 *
 * @param[out] hw_context  hardware manager context
 * @param[in] compression Client compression setting
 * @param[out] settings  rdp settings struct which contains RX res settings to be sent
 * @param[in] head  rdp video_head_index_e index to video head
 *
 */
void connection_manager_populate_client_resolution_values(hwManagerContext * hw_context, const COMPRESSION_MODE compression, rdpSettings * settings, const video_head_index_e head)
{
	settings->head_detected[head] = hw_context->head_detected[head];

	if(MODE_CHECK(compression, LOSSLESS))
	{
		if(FIRST_HEAD == head)
		{
			settings->connection_resolution[head] = hw_context->lossless_egress_res;
		}
		else
		{
			settings->connection_resolution[head] = (videoHead_t){0};
		}
		//No scaling on lossless currently
		settings->unscaled_input_resolution[head] =  (videoHead_t){0};
	}
	if(MODE_CHECK(compression, OPTIMISED))
	{
		if( (hw_context->optimised_path_scaled) && (FIRST_HEAD == head) )
		{
			//TODO MD - Rename to ingress
			settings->unscaled_input_resolution[head] = hw_context->ingress_resolution[head];
		}
		else
		{
			settings->unscaled_input_resolution[head] =  (videoHead_t){0};
		}
		settings->connection_resolution[head] = hw_context->optimised_egress_res[head];
	}
}


#endif