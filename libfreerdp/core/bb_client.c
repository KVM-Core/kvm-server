#include "client.h"
#include "bb_eth.h"
#include <netinet/tcp.h>
#include <system_toe.h>
#include <system_avae.h>
#include <system_utils.h>
#include <capture_layer.h>
#include <freerdp/connection_manager.h>
#include <network.h>

//#define SHARED_MODE_DEBUG

/* Helper: get TCP info (RTT, MSS) directly from the peer's socket.
 * Replaces the old transport_get_info() which no longer exists. */
static BOOL bb_get_tcp_info(freerdp_peer* client, struct tcp_info* info)
{
	socklen_t len = sizeof(*info);
	memset(info, 0, sizeof(*info));
	return getsockopt(client->sockfd, IPPROTO_TCP, TCP_INFO, info, &len) == 0;
}

int client_update_tcp_info(freerdp_peer *client, const char *caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	BOOL audio_enabled = FALSE; //bb_peer_context->settings->AnalogAudio;
	struct tcp_info info;
	int delta, rtt, mss;

	/* No need to set MSS for UDP multicast mode.
	 * RTT is getting set on master channel through
	 * client_update_multicast_info() */
	if (MODE_CHECK(bb_peer_context->connection_mode, MULTICAST_MODE))
		return 1;

	/*
	 * Ensure that TOE connections have been allocated (valid CIDs)
	 * before attempting to update their RTT and MSS values.
	 */
	if (!(bb_peer_context->video_slave_cid || bb_peer_context->audio_slave_cid))
		return 1;

	if (!bb_get_tcp_info(client, &info)) {
		corrib_syslog(LOG_ERR, "%s: Failed to get TCP info for RDP connection to %s\n",
			      __func__, client->hostname);
		return 0;
	}
	rtt = info.tcpi_rtt;
	mss = info.tcpi_snd_mss;

	/*
	 * Update TOE connection MSS only if it hasn't already been set or
	 * if it has shrunk.  Don't allow an MSS increase (see BUG-3148).
	 */
	if ((!bb_peer_context->last_mss || (mss < (int)bb_peer_context->last_mss))) {
		if (su_toe_mss_set(bb_peer_context->video_slave_cid, mss)) {
			corrib_syslog(LOG_ERR, "%s: Failed to update TCP MSS for video channel\n",
				      __func__);
			return 0;
		}
		if (audio_enabled) {
			if (su_toe_mss_set(bb_peer_context->audio_slave_cid, mss)) {
				corrib_syslog(LOG_ERR, "%s: Failed to update TCP MSS for audio channel\n",
					      __func__);
				return 0;
			}
		}
		bb_peer_context->last_mss = mss;
	}

	/* For efficiency, don't update the RTT unless its changed by at least 1/8th */
	delta = abs(rtt - (int)bb_peer_context->last_rtt);
	if (delta >= (int)(bb_peer_context->last_rtt >> 3)) {
		if (su_toe_os_rtt_set(bb_peer_context->video_slave_cid, rtt)) {
			corrib_syslog(LOG_ERR, "%s: Failed to update TCP RTT for video channel\n",
				      __func__);
			return 0;
		}

		if (audio_enabled && su_toe_os_rtt_set(bb_peer_context->audio_slave_cid, rtt)) {
			corrib_syslog(LOG_ERR, "%s: Failed to update TCP RTT for audio channel\n",
				      __func__);
			return 0;
		}
	}
	bb_peer_context->last_rtt = rtt;

	return 1;
}

int client_update_multicast_info(cmContext * cm_context, const char *caller)
{
	struct tcp_info info;
	int delta;
	uint64_t rtt_usec, rtt_max = 0;
	peerNode * node_iterator;

	/* Iterate through all multicast clients and program the highest RTT in
	 * master channel */
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if (node_iterator->client)
		{
			freerdp_peer *client = node_iterator->client;
			bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;

			if (!bb_peer_context)
				continue;

			if (!MODE_CHECK(bb_peer_context->connection_mode, MULTICAST_MODE))
				continue;

			if (!bb_get_tcp_info(client, &info)) {
				corrib_syslog(LOG_ERR, "%s: Failed to get RTT info for client:%s \n",
						__func__, client->hostname);
				continue;
			}
			rtt_usec = info.tcpi_rtt;

			if (rtt_usec > rtt_max)
				rtt_max = rtt_usec;
		}
	}

	/* For efficiency, don't update the RTT unless its changed by at least 1/8th */
	delta = abs((int64_t)rtt_max - cm_context->multicast_last_rtt);
	if (delta >= (cm_context->multicast_last_rtt >> 3)) {
		if (su_toe_os_rtt_set(VIDEO_MASTER_CID, rtt_max)) {
			corrib_syslog(LOG_ERR, "%s: Failed to update RTT for Video Master CT%02X\n",
				      __func__, VIDEO_MASTER_CID);
			return 0;
		}

		if (cm_context->audio_multicast_running && su_toe_os_rtt_set(AUDIO_MASTER_CID, rtt_max)) {
			corrib_syslog(LOG_ERR, "%s: Failed to update RTT for Audio Master CT%02X\n",
				      __func__, AUDIO_MASTER_CID);
			return 0;
		}
	}
	cm_context->multicast_last_rtt = rtt_max;

	return 1;
}

char* client_get_ip(char * IP, const char* interface)
{
	int iSocket = -1;

	if ((iSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		return 0;
	}

	struct if_nameindex* pIndex = if_nameindex();
	struct if_nameindex* pIndex2 = pIndex;

	while ((pIndex != NULL) && (pIndex->if_name != NULL))
	{
		struct ifreq req;

		strncpy(req.ifr_name, pIndex->if_name, IFNAMSIZ);

		if (ioctl(iSocket, SIOCGIFADDR, &req) < 0)
		{
			if (errno == EADDRNOTAVAIL)
			{
				++pIndex;
				continue;
			}
			perror("ioctl");
			close(iSocket);
			return 0;
		}

		if (strcmp(interface, pIndex->if_name) == 0)
		{
			strcpy(IP, inet_ntoa(((struct sockaddr_in*)&req.ifr_addr)->sin_addr));
			break;
		}
		++pIndex;
	}

	if_freenameindex(pIndex2);
	close(iSocket);
	return IP;
}

int client_open_tcp_listening_ports(freerdp_peer* client, cmContext * cm_context, BOOL video_enabled, BOOL audio_enabled, BOOL primary_video, BOOL primary_audio, const char * caller)
{
	char local_ip[25];
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	CAPTURE_LAYER_CONTEXT *capture_context = cm_context->hm_context->capture_context;
	struct tcp_info info;

	if (!bb_get_tcp_info(client, &info)) {
		corrib_syslog(LOG_ERR, "%s: Failed to get TCP info for RDP connection to %s\n",
			      __func__, client->hostname);
		return 0;
	}

	// TODO - can the TOE driver get the local IP directly (e.g. with netlink notifier)?
	su_get_ip_address(local_ip, ETH_INTERFACE);
	if (su_toe_local_ip_set(local_ip))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to set TOE local IP address to '%s'\n",
			      __func__, local_ip);
		return 0;
	}

	if (video_enabled) {
		if (su_toe_fastpath_is_enabled(bb_peer_context->video_slave_cid)) {
			corrib_syslog(LOG_ERR, "%s: Video connection ct%02x is already active\n",
				      __func__, bb_peer_context->video_slave_cid);
			return 0;
		}

		if (su_avae_video_egress_init(bb_peer_context->compression_mode))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to initialise AVAE Video Egress\n",
					__func__);
			return 0;
		}

		if (su_toe_tcp_video_ct_setup(bb_peer_context->video_slave_cid,
				bb_peer_context->compression_mode,
				info.tcpi_rtt,
				info.tcpi_snd_mss,
				TRANSMITTER,
				bb_peer_context->settings->client_technology_type,
				cm_context->server_technology_type))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to set TCP parameters for video channel ct%02x\n",
				      __func__, bb_peer_context->video_slave_cid);
			return 0;
		}

		if (su_toe_tcp_server_connection_start(bb_peer_context->video_slave_cid, BB_DEFAULT_VIDEO_PORT)) {
			corrib_syslog(LOG_ERR, "%s: Failed to start video TCP connection ct%02x\n",
				      __func__, bb_peer_context->video_slave_cid);
			return 0;
		}

#ifdef DUMP_CT_DEBUG
		su_toe_dump_connection_table(bb_peer_context->video_slave_cid, "AFTER PORT LISTENING");
#endif

		corrib_syslog(LOG_INFO, "%s:Launched connection listener for video channel %u on ct%02x\n",
		      caller, bb_peer_context->video_channel, bb_peer_context->video_slave_cid);
	}

	if (audio_enabled) {
		if (su_toe_fastpath_is_enabled(bb_peer_context->audio_slave_cid)) {
			corrib_syslog(LOG_ERR, "%s: Audio connection ct%02x is already active\n",
				      __func__, bb_peer_context->audio_slave_cid);
			return 0;
		}

		if (su_toe_tcp_audio_ct_setup(bb_peer_context->audio_slave_cid,
					      info.tcpi_rtt,
					      info.tcpi_snd_mss,
					      bb_peer_context->settings->client_technology_type,
					      cm_context->server_technology_type))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to set TCP parameters for audio channel ct%02x\n",
				      __func__, bb_peer_context->audio_slave_cid);
			return 0;
		}

		if (primary_audio && !cm_context->audio_multicast_running)
		{
			/*
			 * Most AVAE audio settings are configured just once by
			 * su_avae_init().  The AVAE buffer pointers need to be
			 * reset before starting a new audio stream, but not
			 * when adding secondary municast channels on a shared
			 * connection.
			 */
			if (su_avae_audio_reset())
			{
				corrib_syslog(LOG_ERR, "%s: Failed to reset Audio\n", __func__);
				return 0;
			}

			if (su_avae_audio_egress_buffer_setup(bb_peer_context->audio_slave_cid))
			{
				corrib_syslog(LOG_ERR, "%s: Failed to setup AVAE audio egress buffer\n",
					      __func__);
				return 0;
			}
			if (su_avae_audio_ingress_buffer_setup(bb_peer_context->audio_slave_cid))
			{
				corrib_syslog(LOG_ERR, "%s: Failed to setup AVAE audio ingress buffer\n",
					      __func__);
				return 0;
			}
		}

		if (su_toe_tcp_server_connection_start(bb_peer_context->audio_slave_cid, BB_DEFAULT_AUDIO_PORT)) {
			corrib_syslog(LOG_ERR, "%s: Failed to start audio TCP connection ct%02x\n",
				      __func__, bb_peer_context->audio_slave_cid);
			return 0;
		}

		corrib_syslog(LOG_INFO, "%s:Launched connection listener for audio channel %u on ct%02x\n",
		      caller, bb_peer_context->audio_channel, bb_peer_context->audio_slave_cid);
	}

	bb_peer_context->last_rtt = info.tcpi_rtt;
	bb_peer_context->last_mss = info.tcpi_snd_mss;

	return 1;
}


void client_show_compression_mode(COMPRESSION_MODE mode, const char * caller)
{
	if (mode == PIXEL_PERFECT)
		corrib_syslog(LOG_DEBUG, "%s: Compression Mode: PIXEL_PERFECT\n ", caller);
	else if (mode == LOSSLESS)
		corrib_syslog(LOG_DEBUG, "%s: Compression Mode: LOSSLESS\n ", caller);
	else if (mode == OPTIMISED)
		corrib_syslog(LOG_DEBUG, "%s: Compression Mode: OPTIMISED\n ", caller);
	else
		corrib_syslog(LOG_DEBUG, "%s: Compression Mode: UNKNOWN\n ", caller);
}

int client_start_encoder_video(freerdp_peer* client, cmContext * cm_context, int audio_enable, BOOL extended_desktop, const char * caller)
{
	int ret = 1;
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	CAPTURE_LAYER_CONTEXT *capture_context = cm_context->hm_context->capture_context;
	bool sync_loss_h1 = !(bb_peer_context->settings->connection_resolution[FIRST_HEAD].width &&
	                       bb_peer_context->settings->connection_resolution[FIRST_HEAD].height);
	bool sync_loss_h2 = !(bb_peer_context->settings->connection_resolution[SECOND_HEAD].width &&
	                       bb_peer_context->settings->connection_resolution[SECOND_HEAD].height);
	corrib_syslog_bs(LOG_INFO, "client_start_encoder_video");

	if (bb_peer_context->compression_mode == LOSSLESS) {
		/* Start AVAE video egress in lossless mode */
		if (su_avae_enc_video_egress_lossless_start(bb_peer_context->video_slave_cid))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to start video egress lossless on channel %u\n",
					__func__, bb_peer_context->video_channel);
			ret = 0;
			goto exit;
		}
	} else {
		/* Start AVAE video egress in optimized mode */
		if (su_avae_enc_video_egress_optimized_start(bb_peer_context->video_channel,
					bb_peer_context->video_slave_cid))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to start video egress optimized on channel %u\n",
					__func__, bb_peer_context->video_channel);
			ret = 0;
			goto exit;
		}
	}

	/* Enable TX HW Data Flow Control */
	capture_layer_enable_data_engine_for_mode(capture_context, bb_peer_context->compression_mode);

	if (!sync_loss_h1)
	{
		capture_layer_rate_control_enable(capture_context, FIRST_HEAD, bb_peer_context->compression_mode);
		capture_layer_start_capture(capture_context, FIRST_HEAD, bb_peer_context->compression_mode);
	}

	if (extended_desktop && !sync_loss_h2)
		capture_layer_rate_control_enable(capture_context, SECOND_HEAD, bb_peer_context->compression_mode);
	else
		capture_layer_rate_control_disable(capture_context, SECOND_HEAD, bb_peer_context->compression_mode);

	cm_context->video_municast_running = TRUE;

	if (!sync_loss_h2)
	{
		/* Enable Capture for head 2 */
		capture_layer_start_capture(capture_context, SECOND_HEAD, bb_peer_context->compression_mode);
	}

	corrib_syslog(LOG_INFO, "%s: Started (m)unicast video channel ct%02X\n",
			__func__, bb_peer_context->video_slave_cid);

exit:
#ifdef DUMP_CT_DEBUG
	su_toe_dump_connection_table(bb_peer_context->video_slave_cid, "AFTER STARTING VIDEO");
#endif

	corrib_syslog_es(LOG_INFO, "client_start_encoder_video");
	return ret;
}

int client_start_encoder_video_mu(freerdp_peer* client, cmContext * cm_context, BOOL res_change, BOOL extended_desktop, const char * caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	CAPTURE_LAYER_CONTEXT *capture_context = cm_context->hm_context->capture_context;

	/* Start AVAE video egress data flow */
	if (su_avae_enc_video_egress_mu_start(bb_peer_context->video_channel,
					      bb_peer_context->video_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start video channel %u\n",
			      __func__, bb_peer_context->video_channel);
		return 0;
	}

	/* Enable Capture for second head if needed */
	if (extended_desktop)
		capture_layer_rate_control_enable(capture_context, SECOND_HEAD, bb_peer_context->compression_mode);

	corrib_syslog(LOG_INFO, "%s: Started (m)unicast video channel ct%02X\n",
		      __func__, bb_peer_context->video_slave_cid);
	return 1;
}

int client_start_encoder_audio(freerdp_peer* client, cmContext * cm_context, BOOL shared, const char * caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;

	/* Start Audio core if not already started by Multicast */
	if (!cm_context->audio_multicast_running) {
		if (su_avae_codec_mute_set(false, TRANSMITTER))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to unmute codec\n", __func__);
			return 0;
		}

		if (su_avae_audio_egress_start())
		{
			corrib_syslog(LOG_ERR, "%s: Failed to start audio egress\n", __func__);
			return 0;
		}
		if (!shared)
		{
			if (su_avae_audio_ingress_start())
			{
				corrib_syslog(LOG_ERR, "%s: Failed to start audio ingress\n", __func__);
				return 0;
			}
		}
	}

	if (su_avae_enc_audio_egress_mu_enable(bb_peer_context->audio_channel,
				bb_peer_context->audio_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to enable MU channel\n", __func__);
		return 0;
	}

	cm_context->audio_municast_running = TRUE;

	corrib_syslog(LOG_INFO, "%s: Started (m)unicast audio channel ct%02X\n",
			__func__, bb_peer_context->audio_slave_cid);
	return 1;
}

int client_start_encoder_audio_mu(freerdp_peer* client, const char * caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;

	if (su_avae_enc_audio_egress_mu_enable(bb_peer_context->audio_channel,
					       bb_peer_context->audio_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start audio egress channel %u\n",
			      __func__, bb_peer_context->audio_channel);
		return 0;
	}

	corrib_syslog(LOG_INFO, "%s: Started (m)unicast audio channel ct%02X\n",
		      __func__, bb_peer_context->audio_slave_cid);
	return 1;
}

int client_stop_encoder_video(freerdp_peer *client, cmContext * cm_context, int num_peers, BOOL extended_desktop, BOOL stop_capture, const char *caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	CAPTURE_LAYER_CONTEXT * capture_context = cm_context->hm_context->capture_context;

#ifdef DUMP_CT_DEBUG
	su_toe_dump_connection_table(bb_peer_context->video_slave_cid, "BEFORE STOPPING VIDEO");
#endif

	/* Ensure disabling 2nd head when last DH RX leaving from a shared session */
	if (extended_desktop)
		capture_layer_rate_control_disable(capture_context, SECOND_HEAD, bb_peer_context->compression_mode);

	if (stop_capture)
	{
		capture_layer_stop_capture(capture_context, FIRST_HEAD, bb_peer_context->compression_mode, false);
		capture_layer_stop_capture(capture_context, SECOND_HEAD, bb_peer_context->compression_mode, false);
		/* Disable TX HW Data Flow Control */
		capture_layer_disable_data_engine_for_mode(capture_context, bb_peer_context->compression_mode);
	}

	corrib_syslog(LOG_INFO, "%s: Stopping video channel %u (TOE ct%02x)\n",
			__func__, bb_peer_context->video_channel, bb_peer_context->video_slave_cid);

	if (!su_toe_fastpath_is_enabled(bb_peer_context->video_slave_cid))
		corrib_syslog(LOG_INFO, "%s: video channel %u (TOE ct%02x) was already disabled\n",
				__func__, bb_peer_context->video_channel, bb_peer_context->video_slave_cid);

#ifndef _EMERALD4K
	/* It is necessary to disable the TOE connection BEFORE disabling the channel
	   on the AVAE.  It can happen that the TOE may still be trying to retransmit
	   TX descriptors provided by the AVAE immediately after the AVAE channel is
	   disabled, but those descriptors are discarded by the AVAE when the channel
	   is disabled so retransmits can fail and result in a complete lock-up of
	   the TOE-MAC data pipeline.

	   The channel teardown sequence recommended by CreVinn is as follows:
	   1. Disable the Channel in the TOE CT entry.
	   2. Start polling the DMA_TTX_IN_PROG_CIDS register in the ToE. If the
	   channel being disabled is in any of the in_prog 3 values then keep
	   polling until it isn't.
	   3. Disable the entry in the AVAE MU bitmask
	   4. Do the TOE Tx flush (again if already done in 2).
	*/
	if (su_toe_tcp_ct_disable(bb_peer_context->video_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to disable TOE ct%02x\n",
				__func__, bb_peer_context->video_slave_cid);
	}
#endif
	if (su_avae_enc_video_egress_mu_disable(bb_peer_context->compression_mode,
				bb_peer_context->video_channel))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to disable AVAE video egress channel %u\n",
				__func__, bb_peer_context->video_channel);
	}
	if (stop_capture)
	{
		/* Stop AVAE video egress */
		su_avae_enc_video_egress_stop(bb_peer_context->compression_mode);
	}
	if (su_toe_ct_stop(bb_peer_context->video_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to stop TOE ct%02x\n",
				__func__, bb_peer_context->video_slave_cid);
	}
	if (stop_capture)
		cm_context->video_municast_running = FALSE;

#ifdef DUMP_CT_DEBUG
	su_toe_dump_connection_table(bb_peer_context->video_slave_cid, "AFTER STOPPING VIDEO");
#endif

	corrib_syslog(LOG_INFO, "%s: Stopped (m)unicast video channel ct%02X\n",
		      __func__, bb_peer_context->video_slave_cid);
	return 1;
}

int client_stop_encoder_audio(freerdp_peer* client, cmContext * cm_context, int num_peers, BOOL stop_capture, char *caller)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;

	if (!su_toe_fastpath_is_enabled(bb_peer_context->audio_slave_cid))
		corrib_syslog(LOG_INFO, "%s: Audio channel %u (TOE ct%02x) was already disabled\n",
				__func__, bb_peer_context->audio_channel, bb_peer_context->audio_slave_cid);

#ifndef _EMERALD4K
	if (su_toe_tcp_ct_disable(bb_peer_context->audio_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to disable TOE ct%02x\n",
				__func__, bb_peer_context->audio_slave_cid);
	}
#endif
	if (su_avae_enc_audio_egress_mu_disable(bb_peer_context->audio_channel))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to disable AVAE audio egress channel %u\n",
				__func__, bb_peer_context->audio_channel);
	}

	if (su_toe_ct_stop(bb_peer_context->audio_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to stop TOE ct%02x\n",
				__func__, bb_peer_context->audio_slave_cid);
	}

	/* If no Municast and Multicast left, stop audio core block */
	if (stop_capture && !(cm_context->audio_multicast_running)) {
		corrib_syslog(LOG_INFO, "%s: Stopping Audio core\n", __func__);
		/* Mute the audio codec */
		if (su_avae_codec_mute_set(true, TRANSMITTER))
			corrib_syslog(LOG_ERR, "%s: Failed to mute codec\n", __func__);
		/* Disable the egress data flow in the AVAE */
		if (su_avae_audio_egress_stop())
			corrib_syslog(LOG_ERR, "%s: Failed to stop AVAE audio egress\n", __func__);
		/* Stop AAE audio ingress after tearing down TCP connection */
		if (su_avae_audio_ingress_stop())
			corrib_syslog(LOG_ERR, "%s: Failed to stop AVAE audio ingress\n", __func__);
		/* Finally, flush the AVAE audio */
		if (su_avae_audio_flush())
			corrib_syslog(LOG_WARNING, "%s: Failed to flush AVAE audio\n", __func__);
	}

	/* When all Municast peers have left */
	if (stop_capture)
		cm_context->audio_municast_running = FALSE;

	corrib_syslog(LOG_INFO, "%s: Stopped (m)unicast audio channel ct%02X\n",
		      __func__, bb_peer_context->audio_slave_cid);
	return 1;
}

void client_dump_encoder_toe(char * caller, unsigned int slave_cid)
{
	char command[1000];
	corrib_syslog(LOG_DEBUG, "<<< Dumping %s_%02x.dump >>>>/n", caller, slave_cid);
	sprintf(command, "/opt/blackbox/connection_management/debug_utils/dump_encoder_shared_setup.sh  %s ct%02x %s_%02x.dump",
		caller, slave_cid, caller, slave_cid);
	if (system(command))
		corrib_syslog(LOG_ERR, "%s:CM:%s Command Failed. .\n", __func__, command);
}

int client_master_resume_video(cmContext *cm_context)
{
	if (!su_toe_fastpath_is_enabled(VIDEO_MASTER_CID))
	{
		corrib_syslog(LOG_INFO, "%s: video channel 0x%02x is not enabled\n",
			      __func__, VIDEO_MASTER_CID);
		return 1;
	}

	/* Send UDP SYN packet */
	if (su_toe_send_syn_flag(VIDEO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to send UDP SYN for Video Master\n", __func__);
		return 0;
	}

	corrib_syslog(LOG_INFO, "%s: Resumed multicast video master channel ct%02X\n",
		      __func__, VIDEO_MASTER_CID);
	return 1;
}

int client_master_resume_audio(void)
{
	if (!su_toe_fastpath_is_enabled(AUDIO_MASTER_CID))
	{
		corrib_syslog(LOG_INFO, "%s: audio channel 0x%02x is not enabled\n",
			      __func__, AUDIO_MASTER_CID);
		return 1;
	}

	/* Send UDP SYN packet */
	if (su_toe_send_syn_flag(AUDIO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to send UDP SYN for Audio Master\n", __func__);
		return 0;
	}

	corrib_syslog(LOG_INFO, "%s: Resumed multicast audio master channel ct%02X\n",
		      __func__, AUDIO_MASTER_CID);
	return 1;
}

int client_slave_start_video(freerdp_peer* client)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	char nexthop_mac[32];

	if (su_toe_fastpath_is_enabled(bb_peer_context->video_slave_cid)) {
		corrib_syslog(LOG_ERR, "%s: Fast path already active on cid %u\n",
			      __func__, bb_peer_context->video_slave_cid);
		return 1;
	}

	if (!su_toe_fastpath_is_enabled(VIDEO_MASTER_CID)) {
		corrib_syslog(LOG_ERR, "%s: master connection (ct%02x) not running, unable to start slave\n",
			      __func__, VIDEO_MASTER_CID);
		return 0;
	}

	/* resolve next-hop MAC - differs from peer MAC if connected via router */
	if (su_get_nexthop_mac_address(client->hostname, bb_peer_context->settings->source_macaddr,
				       nexthop_mac))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to resolve next-hop MAC address\n", __func__);
		return 0;
	}

	/* setup TOE multicast shared-connection for specified client */
	if (su_toe_rmc_video_slave_ct_setup(bb_peer_context->video_slave_cid, client->hostname,
					    nexthop_mac, bb_peer_context->video_sequence_number))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to setup multicast video slave channel ct%02X\n",
			      __func__, bb_peer_context->video_slave_cid);
		return 0;
	}

	/* start TOE data flow for the specified client connection */
	if (su_toe_rmc_slave_start(bb_peer_context->video_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start multicast video slave channel ct%02X\n",
			      __func__, bb_peer_context->video_slave_cid);
		return 0;
	}

	corrib_syslog(LOG_INFO, "%s: Started multicast video slave channel ct%02X\n",
		      __func__, bb_peer_context->video_slave_cid);
	return 1;
}

int client_slave_start_audio(freerdp_peer* client)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	char nexthop_mac[32];

	if (su_toe_fastpath_is_enabled(bb_peer_context->audio_slave_cid)) {
		corrib_syslog(LOG_ERR, "%s: Fast path already active on cid %u\n",
			      __func__, bb_peer_context->audio_slave_cid);
		return 1;
	}

	if (!su_toe_fastpath_is_enabled(AUDIO_MASTER_CID)) {
		corrib_syslog(LOG_ERR, "%s: master connection (ct%02x) not running, unable to start slave\n",
			      __func__, AUDIO_MASTER_CID);
		return 0;
	}

	/* resolve next-hop MAC - differs from peer MAC if connected via router */
	if (su_get_nexthop_mac_address(client->hostname, bb_peer_context->settings->source_macaddr,
				       nexthop_mac))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to resolve remote MAC address\n", __func__);
		return 0;
	}

	/* setup TOE multicast shared-connection for specified client */
	if (su_toe_rmc_audio_slave_ct_setup(bb_peer_context->audio_slave_cid, client->hostname,
					    nexthop_mac, bb_peer_context->audio_sequence_number))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to setup multicast audio slave channel ct%02X\n",
			      __func__, bb_peer_context->audio_slave_cid);
		return 0;
	}

	/* start TOE data flow for the specified client connection */
	if (su_toe_rmc_slave_start(bb_peer_context->audio_slave_cid))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start multicast audio slave channel ct%02X\n",
			      __func__, bb_peer_context->audio_slave_cid);
		return 0;
	}

	corrib_syslog(LOG_INFO, "%s: Started multicast audio slave channel ct%02X\n",
		      __func__, bb_peer_context->audio_slave_cid);
	return 1;
}

#ifdef SHARED_MODE_DEBUG
static unsigned int reschange_count = 0;
#endif

void client_start_encoder_slaves(cmContext * cm_context)
{
	peerNode * node_iterator;
	corrib_syslog(LOG_DEBUG, "CM:%s:\n", __func__);
	LIST_FOREACH(node_iterator, &(cm_context->peer_list_head), entries)
	{
		if (node_iterator)
		{
			freerdp_peer* client = node_iterator->client;
			bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;

			if (bb_peer_context->video_slave_cid == 0) {
				/* video_slave_cid may not yet be allocated at early stages of connection setup */
				corrib_syslog(LOG_INFO, "%s:Invalid video_slave_cid, skipping client %s\n",
					      __func__, bb_peer_context->connection_hostname);
				continue;
			}

			if (client_slave_start_video(client))
			{
				corrib_syslog(LOG_DEBUG, "%s:Launched encoder slave start for client in shared mode%s\n",
					      __func__, bb_peer_context->connection_hostname);
				corrib_syslog(LOG_DEBUG, "%s:Launched encoder slave start for client in shared mode%s\n",
					      __func__, bb_peer_context->connection_hostname);
#ifdef SHARED_MODE_DEBUG
				char details[255];
				sprintf(details, "encoder_reschange_%d", reschange_count);
				client_dump_encoder_toe(details, bb_peer_context->video_slave_cid);
#endif
			}
			else
			{
				corrib_syslog(LOG_ERR, "%s:Failed to launch encoder slave start for client in shared mode %s\n",
					      __func__, bb_peer_context->connection_hostname);
			}
		}
		else
		{
			if (cm_context->debug_enabled)
				corrib_syslog(LOG_DEBUG, "%s: Not processing command because we have no connections\n", __func__);
		}
	}
#ifdef SHARED_MODE_DEBUG
	reschange_count++;
#endif
}

int client_slave_stop_video(freerdp_peer* client)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	unsigned cid = bb_peer_context->video_slave_cid;

	if (!su_toe_fastpath_is_enabled(cid))
		corrib_syslog(LOG_INFO, "%s: Multicast video slave channel ct%02X already stopped\n",
			      __func__, cid);

	if (su_toe_ct_stop(cid))
		corrib_syslog(LOG_ERR, "%s: Failed to stop multicast video slave channel ct%02X\n",
			      __func__, cid);

	corrib_syslog(LOG_INFO, "%s: Stopped multicast video slave channel ct%02X\n",
		      __func__, cid);
	return 1;
}

int client_slave_stop_audio(freerdp_peer* client)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	unsigned cid = bb_peer_context->audio_slave_cid;

	if (!su_toe_fastpath_is_enabled(cid))
		corrib_syslog(LOG_INFO, "%s: Multicast audio slave channel ct%02X already stopped\n",
			      __func__, cid);

	if (su_toe_ct_stop(cid))
		corrib_syslog(LOG_ERR, "%s: Failed to stop multicast audio slave channel ct%02X\n",
			      __func__, cid);

	corrib_syslog(LOG_INFO, "%s: Stopped multicast audio slave channel ct%02X\n",
		      __func__, cid);
	return 1;
}

/*
 * Param: Type: 0: Suspend all video slaves
 *             1: Suspend all audio slaves
 *             2: Suspend both video and audio slaves
*/
void client_tear_down_slaves(int type)
{
	unsigned cid, cid_min, cid_max;

	/* Ensure all video and/or audio multicast client connections are stopped */
	if (type == 0)
	{
		cid_min = STARTING_MULTICAST_VIDEO_CID;
		cid_max = STARTING_MULTICAST_VIDEO_CID + MAX_SHARED_CONNECTIONS - 1;
	}
	else if (type == 1)
	{
		cid_min = STARTING_MULTICAST_AUDIO_CID;
		cid_max = STARTING_MULTICAST_AUDIO_CID + MAX_SHARED_CONNECTIONS - 1;
	}
	else
	{
		cid_min = STARTING_MULTICAST_VIDEO_CID;
		cid_max = STARTING_MULTICAST_AUDIO_CID + MAX_SHARED_CONNECTIONS - 1;
	}

	for (cid = cid_min; cid <= cid_max; cid++) {
		if (cid == VIDEO_MASTER_CID || cid == AUDIO_MASTER_CID)
			continue;
		if (su_toe_fastpath_is_enabled(cid))
			su_toe_ct_stop(cid);
	}
}

int client_master_stop(cmContext * cm_context)
{
	CAPTURE_LAYER_CONTEXT *capture_context = cm_context->hm_context->capture_context;

	if (!su_toe_fastpath_is_enabled(VIDEO_MASTER_CID))
		corrib_syslog(LOG_INFO, "%s: Video channel (TOE ct%02x) is already stopped\n",
				__func__, VIDEO_MASTER_CID);

	corrib_syslog(LOG_INFO, "%s: Stopping Accelerated connection for Master Video channel (TOE ct%02x)\n",
			__func__, VIDEO_MASTER_CID);

	/* Disable the video processing */
	capture_layer_stop_capture(capture_context, FIRST_HEAD, LOSSLESS, false);
	/* Disable TX HW Data Flow Control */
	capture_layer_disable_data_engine_for_mode(capture_context, LOSSLESS);

	/* Disable Lossless MU channel */
	if (su_avae_enc_video_egress_mu_disable(LOSSLESS, 0))
		corrib_syslog(LOG_ERR, "%s: Failed to disable Losless MU channel\n", __func__);

	/* Stop AVAE video egress */
	if (su_avae_enc_video_egress_stop(LOSSLESS))
		corrib_syslog(LOG_ERR, "%s: Failed to stop AVAE video egress\n", __func__);

	/* Then disable the data flow in the TOE master channel */
	if (su_toe_ct_stop(VIDEO_MASTER_CID))
		corrib_syslog(LOG_ERR, "%s: Failed to stop TOE ct%02x\n", __func__, VIDEO_MASTER_CID);

	if (su_toe_congestion_ctrl_set(false))
	{
		corrib_syslog(LOG_ERR, "%s:Failed to disable congestion control\n", __func__);
		return 0;
	}

	cm_context->multicast_last_rtt = 0;
	cm_context->video_multicast_running = FALSE;

	return 1;
}

int client_audio_master_stop(cmContext * cm_context)
{
	bool stop_capture = !(cm_context->audio_municast_running);

	corrib_syslog(LOG_INFO, "%s: Stopping Accelerated connection for Master Audio channel %u\n",
			__func__, AUDIO_MASTER_CID);

	if (!su_toe_fastpath_is_enabled(AUDIO_MASTER_CID))
		corrib_syslog(LOG_INFO, "%s: Audio channel (TOE ct%02x) is already stopped\n",
				__func__, AUDIO_MASTER_CID);

	if (su_avae_enc_audio_egress_mu_disable(cm_context->audio_master_mu_channel))
		corrib_syslog(LOG_ERR, "%s: Failed to disable Multicast AVAE audio egress channel %u\n",
				__func__, cm_context->audio_master_mu_channel);

	/* Then disable the data flow in the TOE master channel */
	if (su_toe_ct_stop(AUDIO_MASTER_CID))
		corrib_syslog(LOG_ERR, "%s: Failed to stop TOE ct%02x\n", __func__, AUDIO_MASTER_CID);

	/* If no Municast and Multicast left, stop audio core block */
	if (stop_capture) {
		corrib_syslog(LOG_INFO, "%s: Stopping Audio core\n", __func__);
		/* Mute the audio codec */
		if (su_avae_codec_mute_set(true, TRANSMITTER))
			corrib_syslog(LOG_ERR, "%s: Failed to mute codec\n", __func__);
		/* Disable the egress data flow in the AVAE */
		if (su_avae_audio_egress_stop())
			corrib_syslog(LOG_ERR, "%s: Failed to stop AVAE audio egress\n", __func__);
		/* Finally, flush the AVAE audio */
		if (su_avae_audio_flush())
			corrib_syslog(LOG_ERR, "%s: Failed to flush AVAE audio\n", __func__);
	}

	cm_context->audio_multicast_running = FALSE;

	corrib_syslog(LOG_INFO, "%s: Stopped multicast audio master channel ct%02X\n",
		      __func__, AUDIO_MASTER_CID);
	return 1;
}

/*
 * Param: 1). cmContext *
 *        2). freerdp_peer*
*/
int client_master_start(cmContext * cm_context, freerdp_peer* client)
{
	corrib_syslog_bs(LOG_INFO, "client_master_start");
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	CAPTURE_LAYER_CONTEXT *capture_context = cm_context->hm_context->capture_context;
	bool sync_loss = !(cm_context->hm_context->ingress_resolution[FIRST_HEAD].width &&
	                   cm_context->hm_context->ingress_resolution[FIRST_HEAD].height);
	struct tcp_info info;
	char local_ip[25];

	if (!bb_get_tcp_info(client, &info)) {
		corrib_syslog(LOG_ERR, "%s: Failed to get RTT info for RDP connection to %s\n",
			      __func__, client->hostname);
		return 0;
	}

	if (su_toe_fastpath_is_enabled(VIDEO_MASTER_CID)) {
		corrib_syslog(LOG_ERR, "%s: Fast path already active on cid %u\n",
			      __func__, VIDEO_MASTER_CID);
		return 1;
	}

	su_get_ip_address(local_ip, ETH_INTERFACE);
	if (su_toe_local_ip_set(local_ip))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to set TOE local IP address to '%s'\n",
			      __func__, local_ip);
		return 0;
	}

	if (su_toe_congestion_ctrl_set(true))
	{
		corrib_syslog(LOG_ERR, "%s:Failed to enable congestion control\n", __func__);
		return 0;
	}

	/* Configure TOE multicast master channel */
	if (su_toe_rmc_video_master_ct_setup(VIDEO_MASTER_CID,
					     bb_peer_context->settings->multicast_ip,
					     bb_peer_context->video_sequence_number,
					     info.tcpi_rtt))
	{
		corrib_syslog(LOG_ERR, "%s:Failed to configure TOE for master video channel: %u\n",
			      __func__, VIDEO_MASTER_CID);
		return 0;
	}

	if (su_avae_video_egress_init(LOSSLESS))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to initialise AVAE Video Egress\n", __func__);
		return 0;
	}

	/* Start AVAE video egress in lossless mode */
	if (su_avae_enc_video_egress_lossless_start(VIDEO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start video egress lossless on channel %u\n",
				__func__, VIDEO_MASTER_CID);
		return 0;
	}

	capture_layer_enable_data_engine_for_mode(capture_context, LOSSLESS);
	if (!sync_loss)
	{
		/* NOTE: hardware_manager_start_capture_subsystem() not available in this build.
		 * If needed, add it to hardware_manager.h and hardware_manager.c. */
		hardware_manager_start_capture_subsystem(cm_context->hm_context, FIRST_HEAD, LOSSLESS);
		capture_layer_start_capture(capture_context, FIRST_HEAD, LOSSLESS);
	}
	else
	{
		corrib_syslog(LOG_INFO, "%s: Not starting capture as we are in sync loss\n", __func__);
	}

	/* Start RMC video master */
	if (su_toe_rmc_master_start(VIDEO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start RMC Video master\n", __func__);
		return 0;
	}

	cm_context->video_multicast_running = TRUE;
	corrib_syslog_es(LOG_INFO, "client_master_start");

	return 1;
}

/*
 * Param: 1). cmContext *
 *        2). freerdp_peer*
*/
int client_audio_master_start(cmContext * cm_context, freerdp_peer* client)
{
	bbPeerContext* bb_peer_context = (bbPeerContext*)client->ContextExtra;
	struct tcp_info info;
	char local_ip[25];

	if (!bb_get_tcp_info(client, &info)) {
		corrib_syslog(LOG_ERR, "%s: Failed to get RTT info for RDP connection to %s\n",
			      __func__, client->hostname);
		return 0;
	}

	if (su_toe_fastpath_is_enabled(AUDIO_MASTER_CID)) {
		corrib_syslog(LOG_ERR, "%s: Fast path already active on cid %u\n",
			      __func__, AUDIO_MASTER_CID);
		return 1;
	}

	su_get_ip_address(local_ip, ETH_INTERFACE);
	if (su_toe_local_ip_set(local_ip))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to set TOE local IP address to '%s'\n",
			      __func__, local_ip);
		return 0;
	}

	/* Configure TOE audio multicast master channel */
	if (su_toe_rmc_audio_master_ct_setup(AUDIO_MASTER_CID,
					     bb_peer_context->settings->multicast_ip,
					     bb_peer_context->audio_sequence_number,
					     info.tcpi_rtt))
	{
		corrib_syslog(LOG_ERR, "%s:Failed to configure TOE for master audio channel: %u\n",
				__func__, AUDIO_MASTER_CID);
		return 0;
	}

	/* Start Audio core only if not started in Municast */
	if (!cm_context->audio_municast_running) {
		if (su_avae_audio_reset())
		{
			corrib_syslog(LOG_ERR, "%s: Failed to reset Audio\n", __func__);
			return 0;
		}

		if (su_avae_audio_egress_buffer_setup(AUDIO_MASTER_CID))
		{
			corrib_syslog(LOG_ERR, "%s: Failed to setup AVAE audio buffer egress\n", __func__);
			return 0;
		}

		/* Unmute the audio codec */
		if (su_avae_codec_mute_set(false, TRANSMITTER))
		{
			corrib_syslog(LOG_ERR, "%s: failed to unmute codec\n", __func__);
			return 0;
		}

		/* Start the audio data flow in the AVAE (egress-only for shared mode) */
		if (su_avae_audio_egress_start())
		{
			corrib_syslog(LOG_ERR, "%s: Failed to start audio\n", __func__);
			return 0;
		}
	}

	if (su_avae_enc_audio_egress_mu_enable(cm_context->audio_master_mu_channel, AUDIO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to enable MU channel for Multicast\n", __func__);
		return 0;
	}

	/* Start RMC audio master */
	if (su_toe_rmc_master_start(AUDIO_MASTER_CID))
	{
		corrib_syslog(LOG_ERR, "%s: Failed to start RMC audio master\n", __func__);
		return 0;
	}

	cm_context->audio_multicast_running = TRUE;

	corrib_syslog(LOG_INFO, "%s: Started multicast audio master channel ct%02X\n",
		      __func__, AUDIO_MASTER_CID);
	return 1;
}
