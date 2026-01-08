#ifndef __BB_PEER_H__
#define __BB_PEER_H__

#include <freerdp/config.h>

#include <errno.h>
#include <signal.h>

#include <winpr/winpr.h>
#include <winpr/crt.h>
#include <winpr/cast.h>
#include <winpr/assert.h>
#include <winpr/ssl.h>
#include <winpr/synch.h>
#include <winpr/file.h>
#include <winpr/string.h>
#include <winpr/path.h>
#include <winpr/image.h>
#include <winpr/winsock.h>

#include <freerdp/streamdump.h>
#include <freerdp/transport_io.h>

#include <freerdp/channels/wtsvc.h>
#include <freerdp/channels/channels.h>
#include <freerdp/channels/drdynvc.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/server/rdpsnd.h>
#include <freerdp/settings.h>

#include "bb_ainput.h"
#include "bb_audin.h"
#include "bb_rdpsnd.h"
#include "bb_encomsp.h"

#include "server_peer.h"
#include "bbfreerdp.h"

#include <freerdp/log.h>

#include <corrib_logger.h>

#define TAG SERVER_TAG("sample")

#define SAMPLE_SERVER_USE_CLIENT_RESOLUTION 1
#define SAMPLE_SERVER_DEFAULT_WIDTH 1024
#define SAMPLE_SERVER_DEFAULT_HEIGHT 768

BOOL test_peer_context_new(freerdp_peer* client, rdpContext* ctx);
void test_peer_context_free(freerdp_peer* client, rdpContext* ctx);

DWORD WINAPI bb_peer_loop(LPVOID arg);

#endif /* __BB_PEER_H__ */