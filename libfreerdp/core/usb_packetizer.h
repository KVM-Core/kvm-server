#ifndef __USB_PACKETIZER_H
#define __USB_PACKETIZER_H

#include <freerdp/update.h>
#include <freerdp/utils/list.h>
#include <freerdp/utils/mutex.h>
// #include <freerdp/channels/rdpeusb.h>
#include <freerdp/hardware_manager.h>

#define ALLOCATED_BUFFER_SIZE 262144 //ARPM: 256KiB
#define ALLOCATED_BUFFER_MAX_THRESHOLD 100

typedef struct usb_packetizer_context upContext;

typedef struct usb_command_result upCommandResult;

#define USB_FD_HEADER_TYPE_OFFSET 0
#define USB_FD_HEADER_RESULT_OFFSET 2
#define USB_FD_HEADER_REQUEST_OFFSET 3
#define USB_FD_HEADER_VALUE_OFFSET 4
#define USB_FD_HEADER_INDEX_OFFSET 6
#define USB_FD_HEADER_ADDRESS_OFFSET 8
#define USB_FD_HEADER_ATTRIBUTES_OFFSET 9
#define USB_FD_HEADER_MAX_PACKET_SIZE_OFFSET 10
#define USB_FD_HEADER_DATA_OFFSET 16

struct usb_packetizer_context
{
	freerdp_mutex mutex;
	LIST* data_pool;
	UINT8 count;
};

struct usb_command_result
{
	// sequenceData* sequence_data;
	USB_COMMAND* cmd;
};

upContext* usb_packetizer_new();
upCommandResult up_create_usb_command(upContext* up_context, hwManagerContext* hm_context, int fd);
BOOL up_write_release_usb_command(upContext* up_context, hwManagerContext* hm_context, upCommandResult cmdResult, int fd);
BOOL up_release_usb_command(upContext* up_context, hwManagerContext* hm_context, upCommandResult cmdResult);
void usb_packetizer_free(upContext* up_context);
#endif //__USB_PACKETIZER_H
