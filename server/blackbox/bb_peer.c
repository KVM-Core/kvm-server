#include "bb_peer.h"

#define TAG SERVER_TAG("sample")

#define SAMPLE_SERVER_USE_CLIENT_RESOLUTION 1
#define SAMPLE_SERVER_DEFAULT_WIDTH 1024
#define SAMPLE_SERVER_DEFAULT_HEIGHT 768

struct server_info
{
	BOOL test_dump_rfx_realtime;
	const char* test_pcap_file;
	const char* replay_dump;
	const char* cert;
	const char* key;
};

void test_peer_context_free(freerdp_peer* client, rdpContext* ctx)
{
	serverPeerContext* context = (serverPeerContext*)ctx;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_UNUSED(client);

	if (context)
	{
		winpr_image_free(context->image, TRUE);
		if (context->debug_channel_thread)
		{
			WINPR_ASSERT(context->stopEvent);
			(void)SetEvent(context->stopEvent);
			(void)WaitForSingleObject(context->debug_channel_thread, INFINITE);
			(void)CloseHandle(context->debug_channel_thread);
		}

		Stream_Free(context->s, TRUE);
		free(context->bg_data);
		rfx_context_free(context->rfx_context);
		nsc_context_free(context->nsc_context);

		if (context->debug_channel)
			(void)WTSVirtualChannelClose(context->debug_channel);

		// sf_peer_audin_uninit(context);

// #if defined(CHANNEL_AINPUT_SERVER)
// 		sf_peer_ainput_uninit(context);
// #endif

		// rdpsnd_server_context_free(context->rdpsnd);
		// encomsp_server_context_free(context->encomsp);

		WTSCloseServer(context->vcm);
	}
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
}

BOOL test_peer_context_new(freerdp_peer* client, rdpContext* ctx)
{
	serverPeerContext* context = (serverPeerContext*)ctx;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);
	WINPR_ASSERT(context);
	WINPR_ASSERT(ctx->settings);

	corrib_syslog(LOG_DEBUG, "%s(): line %d\n", __func__);

	context->image = winpr_image_new();
	if (!context->image)
		goto fail;
	if (!(context->rfx_context = rfx_context_new_ex(
	          TRUE, freerdp_settings_get_uint32(ctx->settings, FreeRDP_ThreadingFlags))))
		goto fail;

	if (!rfx_context_reset(context->rfx_context, SAMPLE_SERVER_DEFAULT_WIDTH,
	                       SAMPLE_SERVER_DEFAULT_HEIGHT))
		goto fail;

	const UINT32 rlgr = freerdp_settings_get_uint32(ctx->settings, FreeRDP_RemoteFxRlgrMode);
	rfx_context_set_mode(context->rfx_context, rlgr);

	if (!(context->nsc_context = nsc_context_new()))
		goto fail;

	if (!(context->s = Stream_New(NULL, 65536)))
		goto fail;

	context->icon_x = UINT32_MAX;
	context->icon_y = UINT32_MAX;
	context->vcm = WTSOpenServerA((LPSTR)client->context);

	if (!context->vcm || context->vcm == INVALID_HANDLE_VALUE)
		goto fail;
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
fail:
	test_peer_context_free(client, ctx);
	return FALSE;
}


static BOOL test_peer_init(freerdp_peer* client)
{
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);
	client->ContextSize = sizeof(serverPeerContext);
	client->ContextNew = test_peer_context_new;
	client->ContextFree = test_peer_context_free;
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return freerdp_peer_context_new(client);
}

static wStream* test_peer_stream_init(serverPeerContext* context)
{
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(context);
	WINPR_ASSERT(context->s);
	Stream_Clear(context->s);
	Stream_SetPosition(context->s, 0);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return context->s;
}

static void test_peer_begin_frame(freerdp_peer* client)
{
	rdpUpdate* update = NULL;
	SURFACE_FRAME_MARKER fm = { 0 };
	serverPeerContext* context = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);
	WINPR_ASSERT(client->context);

	update = client->context->update;
	WINPR_ASSERT(update);

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	fm.frameAction = SURFACECMD_FRAMEACTION_BEGIN;
	fm.frameId = context->frame_id;
	WINPR_ASSERT(update->SurfaceFrameMarker);
	update->SurfaceFrameMarker(update->context, &fm);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
}

static void test_peer_end_frame(freerdp_peer* client)
{
	rdpUpdate* update = NULL;
	SURFACE_FRAME_MARKER fm = { 0 };
	serverPeerContext* context = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	update = client->context->update;
	WINPR_ASSERT(update);

	fm.frameAction = SURFACECMD_FRAMEACTION_END;
	fm.frameId = context->frame_id;
	WINPR_ASSERT(update->SurfaceFrameMarker);
	update->SurfaceFrameMarker(update->context, &fm);
	context->frame_id++;
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
}

static BOOL stream_surface_bits_supported(const rdpSettings* settings)
{
	const UINT32 supported =
	    freerdp_settings_get_uint32(settings, FreeRDP_SurfaceCommandsSupported);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	return ((supported & SURFCMDS_STREAM_SURFACE_BITS) != 0);
}

static BOOL test_peer_draw_background(freerdp_peer* client, const RFX_RECT* rect)
{
	SURFACE_BITS_COMMAND cmd = { 0 };
	BOOL ret = FALSE;
	const UINT32 colorFormat = PIXEL_FORMAT_RGB24;
	const size_t bpp = FreeRDPGetBytesPerPixel(colorFormat);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	serverPeerContext* context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	rdpSettings* settings = client->context->settings;
	WINPR_ASSERT(settings);

	rdpUpdate* update = client->context->update;
	WINPR_ASSERT(update);

	const BOOL RemoteFxCodec = freerdp_settings_get_bool(settings, FreeRDP_RemoteFxCodec);
	if (!RemoteFxCodec && !freerdp_settings_get_bool(settings, FreeRDP_NSCodec))
		return FALSE;

	WINPR_ASSERT(freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth) <= UINT16_MAX);
	WINPR_ASSERT(freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight) <= UINT16_MAX);

	wStream* s = test_peer_stream_init(context);
	const size_t size = bpp * rect->width * rect->height;
	if (size == 0)
		return FALSE;

	BYTE* rgb_data = malloc(size);
	if (!rgb_data)
	{
		WLog_ERR(TAG, "Problem allocating memory");
		return FALSE;
	}

	memset(rgb_data, 0xA0, size);

	if (RemoteFxCodec && stream_surface_bits_supported(settings))
	{
		WLog_DBG(TAG, "Using RemoteFX codec");
		rfx_context_set_pixel_format(context->rfx_context, colorFormat);

		WINPR_ASSERT(bpp <= UINT16_MAX);
		RFX_RECT rrect = { .x = 0, .y = 0, .width = rect->width, .height = rect->height };

		if (!rfx_compose_message(context->rfx_context, s, &rrect, 1, rgb_data, rect->width,
		                         rect->height, (UINT32)(bpp * rect->width)))
		{
			goto out;
		}

		const UINT32 RemoteFxCodecId =
		    freerdp_settings_get_uint32(settings, FreeRDP_RemoteFxCodecId);
		WINPR_ASSERT(RemoteFxCodecId <= UINT16_MAX);
		cmd.bmp.codecID = (UINT16)RemoteFxCodecId;
		cmd.cmdType = CMDTYPE_STREAM_SURFACE_BITS;
	}
	else
	{
		WLog_DBG(TAG, "Using NSCodec");
		nsc_context_set_parameters(context->nsc_context, NSC_COLOR_FORMAT, colorFormat);

		WINPR_ASSERT(bpp <= UINT16_MAX);
		nsc_compose_message(context->nsc_context, s, rgb_data, rect->width, rect->height,
		                    (UINT32)(bpp * rect->width));
		const UINT32 NSCodecId = freerdp_settings_get_uint32(settings, FreeRDP_NSCodecId);
		WINPR_ASSERT(NSCodecId <= UINT16_MAX);
		cmd.bmp.codecID = (UINT16)NSCodecId;
		cmd.cmdType = CMDTYPE_SET_SURFACE_BITS;
	}

	cmd.destLeft = rect->x;
	cmd.destTop = rect->y;
	cmd.destRight = rect->x + rect->width;
	cmd.destBottom = rect->y + rect->height;
	cmd.bmp.bpp = 32;
	cmd.bmp.flags = 0;
	cmd.bmp.width = rect->width;
	cmd.bmp.height = rect->height;
	WINPR_ASSERT(Stream_GetPosition(s) <= UINT32_MAX);
	cmd.bmp.bitmapDataLength = (UINT32)Stream_GetPosition(s);
	cmd.bmp.bitmapData = Stream_Buffer(s);

	update->SurfaceBits(update->context, &cmd);
	ret = TRUE;
out:
	free(rgb_data);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return ret;
}

static BOOL bb_peer_post_connect(freerdp_peer* client)
{
	serverPeerContext* context = NULL;
	rdpSettings* settings = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	settings = client->context->settings;
	WINPR_ASSERT(settings);

	/**
	 * This callback is called when the entire connection sequence is done, i.e. we've received the
	 * Font List PDU from the client and sent out the Font Map PDU.
	 * The server may start sending graphics output and receiving keyboard/mouse input after this
	 * callback returns.
	 */
	WLog_DBG(TAG, "Client %s is activated (osMajorType %" PRIu32 " osMinorType %" PRIu32 ")",
	         client->local ? "(local)" : client->hostname,
	         freerdp_settings_get_uint32(settings, FreeRDP_OsMajorType),
	         freerdp_settings_get_uint32(settings, FreeRDP_OsMinorType));

	if (freerdp_settings_get_bool(settings, FreeRDP_AutoLogonEnabled))
	{
		const char* Username = freerdp_settings_get_string(settings, FreeRDP_Username);
		const char* Domain = freerdp_settings_get_string(settings, FreeRDP_Domain);
		WLog_DBG(TAG, " and wants to login automatically as %s\\%s", Domain ? Domain : "",
		         Username);
		/* A real server may perform OS login here if NLA is not executed previously. */
	}

	WLog_DBG(TAG, "");
	WLog_DBG(TAG, "Client requested desktop: %" PRIu32 "x%" PRIu32 "x%" PRIu32 "",
	         freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth),
	         freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight),
	         freerdp_settings_get_uint32(settings, FreeRDP_ColorDepth));
#if (SAMPLE_SERVER_USE_CLIENT_RESOLUTION == 1)

	if (!rfx_context_reset(context->rfx_context,
	                       freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth),
	                       freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight)))
		return FALSE;

	WLog_DBG(TAG, "Using resolution requested by client.");
#else
	client->freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth) =
	    context->rfx_context->width;
	client->freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight) =
	    context->rfx_context->height;
	WLog_DBG(TAG, "Resizing client to %" PRIu32 "x%" PRIu32 "",
	         client->freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth),
	         client->freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight));
	client->update->DesktopResize(client->update->context);
#endif

	// /* A real server should tag the peer as activated here and start sending updates in main loop.
	//  */
	// if (!test_peer_load_icon(client))
	// {
	// 	WLog_DBG(TAG, "Unable to load icon");
	// 	return FALSE;
	// }

	// if (WTSVirtualChannelManagerIsChannelJoined(context->vcm, "rdpdbg"))
	// {
	// 	context->debug_channel = WTSVirtualChannelOpen(context->vcm, WTS_CURRENT_SESSION, "rdpdbg");

	// 	if (context->debug_channel != NULL)
	// 	{
	// 		WLog_DBG(TAG, "Open channel rdpdbg.");

	// 		if (!(context->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
	// 		{
	// 			WLog_ERR(TAG, "Failed to create stop event");
	// 			return FALSE;
	// 		}

	// 		if (!(context->debug_channel_thread =
	// 		          CreateThread(NULL, 0, tf_debug_channel_thread_func, (void*)context, 0, NULL)))
	// 		{
	// 			WLog_ERR(TAG, "Failed to create debug channel thread");
	// 			(void)CloseHandle(context->stopEvent);
	// 			context->stopEvent = NULL;
	// 			return FALSE;
	// 		}
	// 	}
	// }

	// if (WTSVirtualChannelManagerIsChannelJoined(context->vcm, RDPSND_CHANNEL_NAME))
	// {
	// 	sf_peer_rdpsnd_init(context); /* Audio Output */
	// }

	// if (WTSVirtualChannelManagerIsChannelJoined(context->vcm, ENCOMSP_SVC_CHANNEL_NAME))
	// {
	// 	sf_peer_encomsp_init(context); /* Lync Multiparty */
	// }

	// /* Dynamic Virtual Channels */
	// sf_peer_audin_init(context); /* Audio Input */

// #if defined(CHANNEL_AINPUT_SERVER)
// 	sf_peer_ainput_init(context);
// #endif

	/* Return FALSE here would stop the execution of the peer main loop. */
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_activate(freerdp_peer* client)
{
	serverPeerContext* context = NULL;
	rdpSettings* settings = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	settings = client->context->settings;
	WINPR_ASSERT(settings);

	// struct server_info* info = client->ContextExtra;
	// WINPR_ASSERT(info);

	context->_activated = TRUE;
	// PACKET_COMPR_TYPE_8K;
	// PACKET_COMPR_TYPE_64K;
	// PACKET_COMPR_TYPE_RDP6;
	if (!freerdp_settings_set_uint32(settings, FreeRDP_CompressionLevel, PACKET_COMPR_TYPE_RDP8))
		return FALSE;

	// if (info->test_pcap_file != NULL)
	// {
	// 	if (!freerdp_settings_set_bool(settings, FreeRDP_DumpRemoteFx, TRUE))
	// 		return FALSE;

	// 	// if (!bb_peer_dump_rfx(client))
	// 	// 	return FALSE;
	// }
	// else
	// {
		const RFX_RECT rect = {
			.x = 0,
			.y = 0,
			.width = (UINT16)freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth),
			.height = (UINT16)freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight)
		};
		test_peer_begin_frame(client);
		test_peer_draw_background(client, &rect);
		test_peer_end_frame(client);
	// }
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_synchronize_event(rdpInput* input, UINT32 flags)
{
	WINPR_UNUSED(input);
	WINPR_ASSERT(input);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WLog_DBG(TAG, "Client sent a synchronize event (flags:0x%" PRIX32 ")", flags);
	return TRUE;
}

static BOOL bb_peer_keyboard_event(rdpInput* input, UINT16 flags, UINT8 code)
{
	freerdp_peer* client = NULL;
	rdpUpdate* update = NULL;
	rdpContext* context = NULL;
	serverPeerContext* tcontext = NULL;
	rdpSettings* settings = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(input);

	context = input->context;
	WINPR_ASSERT(context);

	client = context->peer;
	WINPR_ASSERT(client);

	settings = context->settings;
	WINPR_ASSERT(settings);

	update = context->update;
	WINPR_ASSERT(update);

	tcontext = (serverPeerContext*)context;
	WINPR_ASSERT(tcontext);

	WLog_DBG(TAG, "Client sent a keyboard event (flags:0x%04" PRIX16 " code:0x%04" PRIX8 ")", flags,
	         code);

	if (((flags & KBD_FLAGS_RELEASE) == 0) && (code == RDP_SCANCODE_KEY_G)) /* 'g' key */
	{
		if (freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth) != 800)
		{
			if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth, 800))
				return FALSE;
			if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight, 600))
				return FALSE;
		}
		else
		{
			if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopWidth,
			                                 SAMPLE_SERVER_DEFAULT_WIDTH))
				return FALSE;
			if (!freerdp_settings_set_uint32(settings, FreeRDP_DesktopHeight,
			                                 SAMPLE_SERVER_DEFAULT_HEIGHT))
				return FALSE;
		}

		if (!rfx_context_reset(tcontext->rfx_context,
		                       freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth),
		                       freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight)))
			return FALSE;

		WINPR_ASSERT(update->DesktopResize);
		update->DesktopResize(update->context);
		tcontext->_activated = FALSE;
	}
	else if (((flags & KBD_FLAGS_RELEASE) == 0) && code == RDP_SCANCODE_KEY_C) /* 'c' key */
	{
		if (tcontext->debug_channel)
		{
			ULONG written = 0;
			if (!WTSVirtualChannelWrite(tcontext->debug_channel, (PCHAR) "test2", 5, &written))
				return FALSE;
		}
	}
	else if (((flags & KBD_FLAGS_RELEASE) == 0) && code == RDP_SCANCODE_KEY_X) /* 'x' key */
	{
		WINPR_ASSERT(client->Close);
		client->Close(client);
	}
	else if (((flags & KBD_FLAGS_RELEASE) == 0) && code == RDP_SCANCODE_KEY_R) /* 'r' key */
	{
		tcontext->audin_open = !tcontext->audin_open;
	}
#if defined(CHANNEL_AINPUT_SERVER)
	else if (((flags & KBD_FLAGS_RELEASE) == 0) && code == RDP_SCANCODE_KEY_I) /* 'i' key */
	{
		tcontext->ainput_open = !tcontext->ainput_open;
	}
#endif
	else if (((flags & KBD_FLAGS_RELEASE) == 0) && code == RDP_SCANCODE_KEY_S) /* 's' key */
	{
	}
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_unicode_keyboard_event(rdpInput* input, UINT16 flags, UINT16 code)
{
	WINPR_UNUSED(input);
	WINPR_ASSERT(input);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WLog_DBG(TAG,
	         "Client sent a unicode keyboard event (flags:0x%04" PRIX16 " code:0x%04" PRIX16 ")",
	         flags, code);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static UINT32 add(UINT32 old, UINT32 max, INT16 diff)
{
	INT64 val = old;
	val += diff;
	if (val > max)
		val = val % max;
	else if (val < 0)
		val = max - val;
	return WINPR_ASSERTING_INT_CAST(uint32_t, val);
}

static BOOL bb_peer_rel_mouse_event(rdpInput* input, UINT16 flags, INT16 xDelta, INT16 yDelta)
{
	WINPR_UNUSED(flags);
	WINPR_ASSERT(input);
	WINPR_ASSERT(input->context);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	const UINT32 w = freerdp_settings_get_uint32(input->context->settings, FreeRDP_DesktopWidth);
	const UINT32 h = freerdp_settings_get_uint32(input->context->settings, FreeRDP_DesktopHeight);

	static UINT32 xpos = 0;
	static UINT32 ypos = 0;

	xpos = add(xpos, w, xDelta);
	ypos = add(ypos, h, yDelta);

	WLog_DBG(TAG,
	         "Client sent a relative mouse event (flags:0x%04" PRIX16 " pos:%" PRId16 ",%" PRId16
	         ")",
	         flags, xDelta, yDelta);
	test_peer_draw_icon(input->context->peer, xpos + 10, ypos);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_extended_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	WINPR_UNUSED(flags);
	WINPR_ASSERT(input);
	WINPR_ASSERT(input->context);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WLog_DBG(TAG,
	         "Client sent an extended mouse event (flags:0x%04" PRIX16 " pos:%" PRIu16 ",%" PRIu16
	         ")",
	         flags, x, y);
	test_peer_draw_icon(input->context->peer, x + 10, y);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_refresh_rect(rdpContext* context, BYTE count, const RECTANGLE_16* areas)
{
	WINPR_UNUSED(context);
	WINPR_ASSERT(context);
	WINPR_ASSERT(areas || (count == 0));
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WLog_DBG(TAG, "Client requested to refresh:");

	for (BYTE i = 0; i < count; i++)
	{
		WLog_DBG(TAG, "  (%" PRIu16 ", %" PRIu16 ") (%" PRIu16 ", %" PRIu16 ")", areas[i].left,
		         areas[i].top, areas[i].right, areas[i].bottom);
	}

    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_suppress_output(rdpContext* context, BYTE allow, const RECTANGLE_16* area)
{
	WINPR_UNUSED(context);
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	if (allow > 0)
	{
		WINPR_ASSERT(area);
		WLog_DBG(TAG,
		         "Client restore output (%" PRIu16 ", %" PRIu16 ") (%" PRIu16 ", %" PRIu16 ").",
		         area->left, area->top, area->right, area->bottom);
	}
	else
	{
		WLog_DBG(TAG, "Client minimized and suppress output.");
	}
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static int hook_peer_write_pdu(rdpTransport* transport, wStream* s)
{
	UINT64 ts = 0;
	wStream* ls = NULL;
	UINT64 last_ts = 0;
	size_t offset = 0;
	UINT32 flags = 0;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	rdpContext* context = transport_get_context(transport);

	WINPR_ASSERT(context);
	WINPR_ASSERT(s);

	freerdp_peer* client = context->peer;
	WINPR_ASSERT(client);

	serverPeerContext* peerCtx = (serverPeerContext*)client->context;
	WINPR_ASSERT(peerCtx);
	WINPR_ASSERT(peerCtx->io.WritePdu);

	/* Let the client authenticate.
	 * After that is done, we stop the normal operation and send
	 * a previously recorded session PDU by PDU to the client.
	 *
	 * This is fragile and the connecting client needs to use the same
	 * configuration as the one that recorded the session!
	 */
	CONNECTION_STATE state = freerdp_get_state(context);
	if (state < CONNECTION_STATE_NEGO)
		return peerCtx->io.WritePdu(transport, s);

	ls = Stream_New(NULL, 4096);
	if (!ls)
		goto fail;

	while (stream_dump_get(context, &flags, ls, &offset, &ts) > 0)
	{
		int rc = 0;
		/* Skip messages from client. */
		if (flags & STREAM_MSG_SRV_TX)
		{
			if ((last_ts > 0) && (ts > last_ts))
			{
				UINT64 diff = ts - last_ts;
				while (diff > 0)
				{
					UINT32 d = diff > UINT32_MAX ? UINT32_MAX : (UINT32)diff;
					diff -= d;
					Sleep(d);
				}
			}
			last_ts = ts;
			rc = peerCtx->io.WritePdu(transport, ls);
			if (rc < 0)
				goto fail;
		}
		Stream_SetPosition(ls, 0);
	}
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
fail:
	Stream_Free(ls, TRUE);
	return -1;
}

static int open_icon(wImage* img)
{
	char* paths[] = { SAMPLE_RESOURCE_ROOT, "." };
	const char* names[] = { "test_icon.webp", "test_icon.png", "test_icon.jpg", "test_icon.bmp" };
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	for (size_t x = 0; x < ARRAYSIZE(paths); x++)
	{
		const char* path = paths[x];
		if (!winpr_PathFileExists(path))
			continue;

		for (size_t y = 0; y < ARRAYSIZE(names); y++)
		{
			const char* name = names[y];
			char* file = GetCombinedPath(path, name);
			int rc = winpr_image_read(img, file);
			free(file);
			if (rc > 0)
				return rc;
		}
	}
	WLog_ERR(TAG, "Unable to open test icon");
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return -1;
}

static BOOL test_peer_load_icon(freerdp_peer* client)
{
	serverPeerContext* context = NULL;
	rdpSettings* settings = NULL;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	settings = client->context->settings;
	WINPR_ASSERT(settings);

	if (!freerdp_settings_get_bool(settings, FreeRDP_RemoteFxCodec) &&
	    !freerdp_settings_get_bool(settings, FreeRDP_NSCodec))
	{
		WLog_ERR(TAG, "Client doesn't support RemoteFX or NSCodec");
		return FALSE;
	}

	int rc = open_icon(context->image);
	if (rc <= 0)
		goto out_fail;

	/* background with same size, which will be used to erase the icon from old position */
	if (!(context->bg_data = calloc(context->image->height, 3ULL * context->image->width)))
		goto out_fail;

	memset(context->bg_data, 0xA0, 3ULL * context->image->height * context->image->width);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
out_fail:
	context->bg_data = NULL;
	return FALSE;
}

static void test_send_cursor_update(freerdp_peer* client, UINT32 x, UINT32 y)
{
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);
	serverPeerContext* context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);

	rdpSettings* settings = client->context->settings;

	const BOOL RemoteFxCodec = freerdp_settings_get_bool(settings, FreeRDP_RemoteFxCodec);

	if (context->image->width < 1 || !context->_activated)
		return;

	RFX_RECT rect = { .x = 0,
		              .y = 0,
		              .width = WINPR_ASSERTING_INT_CAST(UINT16, context->image->width),
		              .height = WINPR_ASSERTING_INT_CAST(UINT16, context->image->height) };

	const UINT32 w = freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth);
	const UINT32 h = freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight);
	if (context->icon_x + context->image->width > w)
		return;
	if (context->icon_y + context->image->height > h)
		return;
	if (x + context->image->width > w)
		return;
	if (y + context->image->height > h)
		return;

	SURFACE_BITS_COMMAND cmd = { 0 };
	if (RemoteFxCodec && stream_surface_bits_supported(settings))
	{
		const UINT32 RemoteFxCodecId =
		    freerdp_settings_get_uint32(settings, FreeRDP_RemoteFxCodecId);
		WINPR_ASSERT(RemoteFxCodecId <= UINT16_MAX);
		cmd.bmp.codecID = (UINT16)RemoteFxCodecId;
		cmd.cmdType = CMDTYPE_STREAM_SURFACE_BITS;
	}
	else
	{
		const UINT32 NSCodecId = freerdp_settings_get_uint32(settings, FreeRDP_NSCodecId);
		WINPR_ASSERT(NSCodecId <= UINT16_MAX);
		cmd.bmp.codecID = (UINT16)NSCodecId;
		cmd.cmdType = CMDTYPE_SET_SURFACE_BITS;
	}

	wStream* s = test_peer_stream_init(context);

	{
		const UINT32 colorFormat =
		    context->image->bitsPerPixel > 24 ? PIXEL_FORMAT_BGRA32 : PIXEL_FORMAT_BGR24;

		if (RemoteFxCodec)
		{
			rfx_context_set_pixel_format(context->rfx_context, colorFormat);
			rfx_compose_message(context->rfx_context, s, &rect, 1, context->image->data, rect.width,
			                    rect.height, context->image->scanline);
		}
		else
		{
			nsc_context_set_parameters(context->nsc_context, NSC_COLOR_FORMAT, colorFormat);
			nsc_compose_message(context->nsc_context, s, context->image->data, rect.width,
			                    rect.height, context->image->scanline);
		}
	}

	cmd.destLeft = x;
	cmd.destTop = y;
	cmd.destRight = x + rect.width;
	cmd.destBottom = y + rect.height;
	cmd.bmp.bpp = 32;
	cmd.bmp.width = rect.width;
	cmd.bmp.height = rect.height;
	cmd.bmp.bitmapDataLength = (UINT32)Stream_GetPosition(s);
	cmd.bmp.bitmapData = Stream_Buffer(s);

	rdpUpdate* update = client->context->update;
	WINPR_ASSERT(update);
	WINPR_ASSERT(update->SurfaceBits);
	update->SurfaceBits(update->context, &cmd);
	context->icon_x = x;
	context->icon_y = y;
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
}

void test_peer_draw_icon(freerdp_peer* client, UINT32 x, UINT32 y)
{
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	rdpSettings* settings = client->context->settings;
	WINPR_ASSERT(settings);

	if (freerdp_settings_get_bool(settings, FreeRDP_DumpRemoteFx))
		return;

	test_peer_begin_frame(client);

	serverPeerContext* context = (serverPeerContext*)client->context;
	if ((context->icon_x != UINT32_MAX) && (context->icon_y != UINT32_MAX))
	{
		RFX_RECT rect = { .x = WINPR_ASSERTING_INT_CAST(uint16_t, context->icon_x),
			              .y = WINPR_ASSERTING_INT_CAST(uint16_t, context->icon_y),
			              .width = WINPR_ASSERTING_INT_CAST(uint16_t, context->image->width),
			              .height = WINPR_ASSERTING_INT_CAST(uint16_t, context->image->height) };

		test_peer_draw_background(client, &rect);
	}
	test_send_cursor_update(client, x, y);
	test_peer_end_frame(client);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
}

static BOOL test_sleep_tsdiff(UINT32* old_sec, UINT32* old_usec, UINT32 new_sec, UINT32 new_usec)
{
	INT64 sec = 0;
	INT64 usec = 0;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(old_sec);
	WINPR_ASSERT(old_usec);

	if ((*old_sec == 0) && (*old_usec == 0))
	{
		*old_sec = new_sec;
		*old_usec = new_usec;
		return TRUE;
	}

	sec = new_sec - *old_sec;
	usec = new_usec - *old_usec;

	if ((sec < 0) || ((sec == 0) && (usec < 0)))
	{
		WLog_ERR(TAG, "Invalid time stamp detected.");
		return FALSE;
	}

	*old_sec = new_sec;
	*old_usec = new_usec;

	while (usec < 0)
	{
		usec += 1000000;
		sec--;
	}

	if (sec > 0)
		Sleep((DWORD)sec * 1000);

	if (usec > 0)
		USleep((DWORD)usec);
    corrib_syslog(LOG_DEBUG, "%s(): end\n", __func__);
	return TRUE;
}

static BOOL bb_peer_mouse_event(rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
	WINPR_UNUSED(flags);
	WINPR_ASSERT(input);
	WINPR_ASSERT(input->context);

	WLog_DBG(TAG, "Client sent a mouse event (flags:0x%04" PRIX16 " pos:%" PRIu16 ",%" PRIu16 ")",
	         flags, x, y);
	test_peer_draw_icon(input->context->peer, x + 10, y);
	return TRUE;
}

DWORD WINAPI bb_peer_loop(LPVOID arg)
{
#if 1

	BOOL rc = 0;
	DWORD error = CHANNEL_RC_OK;
	HANDLE handles[MAXIMUM_WAIT_OBJECTS] = { 0 };
	DWORD count = 0;
	DWORD status = 0;
	serverPeerContext* context = NULL;
	rdpSettings* settings = NULL;
	rdpInput* input = NULL;
	rdpUpdate* update = NULL;
	freerdp_peer* client = (freerdp_peer*)arg;
    corrib_syslog(LOG_DEBUG, "%s(): begin\n", __func__);
	WINPR_ASSERT(client);

	// struct server_info* info = client->ContextExtra;
	// WINPR_ASSERT(info);

	// if (!test_peer_init(client))
	// {
	// 	freerdp_peer_free(client);
	// 	return 0;
	// }

	

	/* Initialize the real server settings here */
	WINPR_ASSERT(client->context);
	settings = client->context->settings;
	WINPR_ASSERT(settings);
	// if (info->replay_dump)
	// {
	// 	if (!freerdp_settings_set_bool(settings, FreeRDP_TransportDumpReplay, TRUE) ||
	// 	    !freerdp_settings_set_string(settings, FreeRDP_TransportDumpFile, info->replay_dump))
	// 		goto fail;
	// }

	// rdpPrivateKey* key = freerdp_key_new_from_file_enc(/*info->key*/"/opt/blackbox/shfreerdp/server.key", NULL);
	// if (!key)
	// 	goto fail;
	// if (!freerdp_settings_set_pointer_len(settings, FreeRDP_RdpServerRsaKey, key, 1))
	// 	goto fail;
	// rdpCertificate* cert = freerdp_certificate_new_from_file(/*info->cert*/"/opt/blackbox/shfreerdp/server.crt");
	// if (!cert)
	// 	goto fail;
	// if (!freerdp_settings_set_pointer_len(settings, FreeRDP_RdpServerCertificate, cert, 1))
	// 	goto fail;

	// if (!freerdp_settings_set_bool(settings, FreeRDP_RdpSecurity, TRUE))
	// 	goto fail;
	// if (!freerdp_settings_set_bool(settings, FreeRDP_TlsSecurity, TRUE))
	// 	goto fail;
	// if (!freerdp_settings_set_bool(settings, FreeRDP_NlaSecurity, FALSE))
	// 	goto fail;
	// if (!freerdp_settings_set_uint32(settings, FreeRDP_EncryptionLevel,
	//                                  ENCRYPTION_LEVEL_CLIENT_COMPATIBLE))
	// 	goto fail;
	// /*  ENCRYPTION_LEVEL_HIGH; */
	// /*  ENCRYPTION_LEVEL_LOW; */
	// /*  ENCRYPTION_LEVEL_FIPS; */
	// if (!freerdp_settings_set_bool(settings, FreeRDP_RemoteFxCodec, TRUE))
	// 	goto fail;
	// if (!freerdp_settings_set_bool(settings, FreeRDP_NSCodec, TRUE) ||
	//     !freerdp_settings_set_uint32(settings, FreeRDP_ColorDepth, 32))
	// 	goto fail;

	// if (!freerdp_settings_set_bool(settings, FreeRDP_SuppressOutput, TRUE))
	// 	goto fail;
	// if (!freerdp_settings_set_bool(settings, FreeRDP_RefreshRect, TRUE))
	// 	goto fail;
	// if (!freerdp_settings_set_bool(settings, FreeRDP_HasRelativeMouseEvent, TRUE))
	// 	goto fail;
     corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	client->PostConnect = bb_peer_post_connect;
	client->Activate = bb_peer_activate;

	WINPR_ASSERT(client->context);
	input = client->context->input;
	WINPR_ASSERT(input);
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	input->SynchronizeEvent = bb_peer_synchronize_event;
	input->KeyboardEvent = bb_peer_keyboard_event;
	input->UnicodeKeyboardEvent = bb_peer_unicode_keyboard_event;
	input->MouseEvent = bb_peer_mouse_event;
	input->RelMouseEvent = bb_peer_rel_mouse_event;
	input->ExtendedMouseEvent = bb_peer_extended_mouse_event;
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	update = client->context->update;
	WINPR_ASSERT(update);
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	update->RefreshRect = bb_peer_refresh_rect;
	update->SuppressOutput = bb_peer_suppress_output;
	if (!freerdp_settings_set_uint32(settings, FreeRDP_MultifragMaxRequestSize,
	                                 0xFFFFFF /* FIXME */))
		goto fail;
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	// WINPR_ASSERT(client->Initialize);
	// rc = client->Initialize(client);
	// if (!rc)
	// 	goto fail;

	context = (serverPeerContext*)client->context;
	WINPR_ASSERT(context);
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	// if (info->replay_dump)
	// {
	// 	const rdpTransportIo* cb = freerdp_get_io_callbacks(client->context);
	// 	rdpTransportIo replay;

	// 	WINPR_ASSERT(cb);
	// 	replay = *cb;
	// 	context->io = *cb;
	// 	replay.WritePdu = hook_peer_write_pdu;
	// 	freerdp_set_io_callbacks(client->context, &replay);
	// }
corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
	corrib_syslog(LOG_DEBUG, "We've got a client %s\n", client->local ? "(local)" : client->hostname);
	while (error == CHANNEL_RC_OK)
	{
        // corrib_syslog(LOG_DEBUG, "%s(): begin while @ %d\n", __func__, __LINE__);
		count = 0;
		{
			WINPR_ASSERT(client->GetEventHandles);
			DWORD tmp = client->GetEventHandles(client, &handles[count], 32 - count);

			if (tmp == 0)
			{
				WLog_ERR(TAG, "Failed to get FreeRDP transport event handles");
				break;
			}

			count += tmp;
		}
// corrib_syslog(LOG_DEBUG, "%s(): at line %d\n", __func__, __LINE__);
		HANDLE channelHandle = WTSVirtualChannelManagerGetEventHandle(context->vcm);
		handles[count++] = channelHandle;
		status = WaitForMultipleObjects(count, handles, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			WLog_ERR(TAG, "WaitForMultipleObjects failed (errno: %d)", errno);
			break;
		}

		WINPR_ASSERT(client->CheckFileDescriptor);
		if (client->CheckFileDescriptor(client) != TRUE)
			break;

		if (WTSVirtualChannelManagerCheckFileDescriptor(context->vcm) != TRUE)
			break;

		/* Handle dynamic virtual channel initializations */
		if (WTSVirtualChannelManagerIsChannelJoined(context->vcm, DRDYNVC_SVC_CHANNEL_NAME))
		{
			switch (WTSVirtualChannelManagerGetDrdynvcState(context->vcm))
			{
				case DRDYNVC_STATE_NONE:
					break;

				case DRDYNVC_STATE_INITIALIZED:
					break;

				case DRDYNVC_STATE_READY:

					// /* Here is the correct state to start dynamic virtual channels */
					// if (sf_peer_audin_running(context) != context->audin_open)
					// {
					// 	if (!sf_peer_audin_running(context))
					// 		sf_peer_audin_start(context);
					// 	else
					// 		sf_peer_audin_stop(context);
					// }

// #if defined(CHANNEL_AINPUT_SERVER)
// 					if (sf_peer_ainput_running(context) != context->ainput_open)
// 					{
// 						if (!sf_peer_ainput_running(context))
// 							sf_peer_ainput_start(context);
// 						else
// 							sf_peer_ainput_stop(context);
// 					}
// #endif

					break;

				case DRDYNVC_STATE_FAILED:
				default:
					break;
			}
		}
        // corrib_syslog(LOG_DEBUG, "%s(): end while @ %d\n", __func__, __LINE__);
	}

	corrib_syslog(LOG_DEBUG, "Client %s disconnected.\n", client->local ? "(local)" : client->hostname);

	WINPR_ASSERT(client->Disconnect);
	client->Disconnect(client);
fail:
	freerdp_peer_context_free(client);
	freerdp_peer_free(client);
	return error;
#endif
}
