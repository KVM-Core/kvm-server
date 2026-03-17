#ifndef __BB_CLIENT_H
#define __BB_CLIENT_H
#include "rdp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <freerdp/utils/stream.h>

int client_update_tcp_info(freerdp_peer *client, const char *caller);
int client_update_multicast_info(cmContext * cm_context, const char *caller);
int client_open_tcp_listening_ports(freerdp_peer* client, cmContext * cm_context, BOOL video_enabled, BOOL audio_enabled, BOOL primary_video, BOOL primary_audio, const char * caller);

int client_start_encoder_video(freerdp_peer* client, cmContext * cm_context, int audio_enable, BOOL extended_desktop, const char * caller);
int client_start_encoder_audio(freerdp_peer* client, cmContext * cm_context, BOOL shared, const char * caller);

void client_show_compression_mode(COMPRESSION_MODE mode, const char * caller);
int client_start_encoder_video_mu(freerdp_peer* client, cmContext * cm_context, BOOL res_change, BOOL extended_desktop, const char * caller);

int client_stop_encoder_video(freerdp_peer *client, cmContext * cm_context, int num_peers, BOOL extended_desktop, BOOL stop_capture, const char *caller);
int client_stop_encoder_audio(freerdp_peer* client, cmContext * cm_context, int num_peers, BOOL stop_capture, char *caller);

int client_slave_start_video(freerdp_peer* client);
int client_slave_start_audio(freerdp_peer* client);
int client_slave_stop_video(freerdp_peer* client);
int client_slave_stop_audio(freerdp_peer* client);
int client_master_start(cmContext * cm_context, freerdp_peer* client);
int client_audio_master_start(cmContext * cm_context, freerdp_peer* client);
void client_start_encoder_slaves(cmContext * cm_context);
void client_tear_down_slaves(int type);
int client_master_resume_video(cmContext *cm_context);
int client_master_resume_audio(void);
void client_dump_encoder_toe(char * caller, unsigned int slave_cid);
int client_master_stop(cmContext * cm_context);
int client_audio_master_stop(cmContext * cm_context);

#endif
