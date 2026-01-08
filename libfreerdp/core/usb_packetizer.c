/*
 * Brian Quinn
 * Copyright: Cloudium Systems 2015
 */
#include "usb_packetizer.h"
#include <freerdp/hardware_manager.h>
#include <freerdp/server/rdpeusb.h>

#include <stdio.h>
#include <string.h>




//#define LOW_LEVEL_LAYER_DEBUG
static void up_generate_statistics(hwManagerContext* context);

/*
 * Constructor
 */
upContext* usb_packetizer_new()
{
	upContext * up_context = xnew(upContext, __func__);
	if ( up_context != NULL ) {
		up_context->data_pool = list_new();
		up_context->mutex = freerdp_mutex_new();
	}
	return up_context;
}

/*
 * Destructor
 */
void usb_packetizer_free(upContext * up_context)
{
	uint8* data = NULL;
	if ( up_context != NULL ) {
		if ( up_context->data_pool != NULL ) {
			freerdp_mutex_lock(up_context->mutex);
			while ( ( data = (uint8*) list_dequeue(up_context->data_pool) ) != NULL ) {
				//corrib_syslog(LOG_DEBUG, "%s(): list_dequeue: up_context->data_pool->count=%d\n", __func__, up_context->data_pool->count);
				up_context->count--;
				xfree(data,__func__);
			}
			freerdp_mutex_unlock(up_context->mutex);
			list_free(up_context->data_pool);
		}
		if ( up_context->mutex ) {
			freerdp_mutex_free(up_context->mutex);
		}
		xfree(up_context,__func__);
	}
}

upCommandResult up_create_usb_command(upContext* up_context, hwManagerContext* hm_context, int fd)
{
	uint8* data_ptr = NULL;
	int readSize = 0;
	upCommandResult cmdResult;

	memset ( &cmdResult, 0x00, sizeof(cmdResult) );
	/* arpm: 256KiB Memory allocation */
#ifdef MEMORY_LEAK_DEBUG            
        corrib_syslog ( LOG_DEBUG, "%s(): Allocating data_ptr\n", __func__);
#endif
	data_ptr = (uint8*) xzalloc (ALLOCATED_BUFFER_SIZE, __func__);
	if ( data_ptr == NULL ) {
		/* what are the implications of these error */
		corrib_syslog(LOG_ERR, "%s: Could not allocate memory to usb command buffer, fatal error, do we have a memory leak\n ", __func__);
	}
	else {
		readSize = read( fd, data_ptr, ALLOCATED_BUFFER_SIZE );
		if ( readSize < 0 ) { /* We got an error reading the USB fd */
			if(!((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))) {

				corrib_syslog(LOG_ERR, "%s: We encountered an error which reading USB data from the kernel, maybe someone removed a device\n", __func__);

			}
			xfree(data_ptr, __func__);
		}
		else if ( readSize >= USB_FD_HEADER_DATA_OFFSET ) { /* Everything went OK */
			if(hm_context) {
				hm_context->usbStatistics.total_bytes_sent_usb_egress += readSize;
				up_generate_statistics(hm_context);
			}
			cmdResult.sequence_data = (sequenceData *) data_ptr;
			if ( cmdResult.sequence_data->maxPacketSize > ( ALLOCATED_BUFFER_SIZE - sizeof(sequenceData) ) ) {

				corrib_syslog(LOG_ERR,"%s: Not enough buffer to accomodate the command header, data received from the kernel was larger than expected \n", __func__);
			}

#ifdef MEMORY_LEAK_DEBUG            
            corrib_syslog ( LOG_DEBUG, "%s(): Creating cmdResult\n", __func__);
#endif
			cmdResult.cmd = usb_command_new( data_ptr + USB_FD_HEADER_DATA_OFFSET );
			cmdResult.cmd->dataLength = readSize - USB_FD_HEADER_DATA_OFFSET;

#ifdef CONFIG_USB_BLACKBOX_USBR_ENABLE_PROFILING
			if (cmdResult.sequence_data != NULL) {
				/*
					This is the right place to add timestamps DRIVER->APP direction
				*/
				if ((cmdResult.sequence_data->type == USBSQT_TRANSFER_OUT_REQUEST) &&
					// cmdResult.sequence_data->address it timestamps all bulk requests to any endpoint
					((cmdResult.sequence_data->attributes & 0x03) == USB_ENDPOINT_XFER_BULK) &&
					(cmdResult.sequence_data->maxPacketSize > 0))
				{
					// TIMESTAMP #2
					usbr_profiling_timestamp(cmdResult.cmd->data, cmdResult.cmd->dataLength, 2);
				}
			}
#endif

#ifdef LOW_LEVEL_LAYER_DEBUG
			corrib_syslog(LOG_DEBUG, "%s(): rd <- f_virtual {type=0x%02x result:0x%02x request:0x%04x value:0x%04x index:0x%04x address:0x%02x attributes:0x%02x maxPktSize:0x%08x}\n",
																																				__func__,
																																				cmdResult.sequence_data->type,
																																				cmdResult.sequence_data->result,
																																				cmdResult.sequence_data->request,
																																				cmdResult.sequence_data->value,
																																				cmdResult.sequence_data->index,
																																				cmdResult.sequence_data->address,
																																				cmdResult.sequence_data->attributes,
																																				cmdResult.sequence_data->maxPacketSize );
			if ( cmdResult.cmd != NULL  ) {
				fprintf (stderr, "%s(): usb_cmd=[", __func__);
				for (int index = 0; index < cmdResult.cmd->dataLength; index ++ ) {
					fprintf (stderr, " %02x", cmdResult.cmd->data[index]);
				}
				fprintf (stderr, " ]hex usb_cmd_length: %d\n", cmdResult.cmd->dataLength);
			}
			else {
				fprintf (stderr, "%s(): cmdRcmdResult->cmd is null\n", __func__);
			}
#endif
		}
		else {
			corrib_syslog(LOG_ERR, "%s: Kernel didn't provide enough data to decode USBR command. It will be discarded.\n", __func__);
			xfree(data_ptr, __func__);
		}
	}
	return cmdResult;
}

static void up_generate_statistics(hwManagerContext* context)
{
	uint32 time_difference;
	uint32 current_time=sh_log_get_mstime();
	unsigned long usb_egress_Kbytes_processed;
	unsigned long usb_ingress_Kbytes_processed;

	time_difference = current_time - context->usbStatistics.previous_time_usb;
	//corrib_syslog(LOG_DEBUG,"H1: bs=%u tbs=%llu\n",size,context->usbStatistics.total_bytes_sent_head1 );
	if(time_difference > 60000) // more than a minute has elapsed
	{
		//Egress
		usb_egress_Kbytes_processed = (unsigned long)(context->usbStatistics.total_bytes_sent_usb_egress/1000);
		context->usbStatistics.usb_egress_Kbytes_per_second = (float)((float)usb_egress_Kbytes_processed/60);

		if(context->usbStatistics.moving_average_usb_egress == 0)
		{
			context->usbStatistics.moving_average_usb_egress = context->usbStatistics.usb_egress_Kbytes_per_second;
			//corrib_syslog(LOG_DEBUG,"%s:resetting avg=%f   \t",__func__,context->usbStatistics.moving_average_video_head1);
		}
		context->usbStatistics.moving_average_usb_egress = hw_manager_get_rolling_average(context->usbStatistics.moving_average_usb_egress,context->usbStatistics.usb_egress_Kbytes_per_second,2);
		if(context->usbStatistics.usb_egress_Kbytes_per_second < context->usbStatistics.min_usb_egress_Kbytes_per_second )
			context->usbStatistics.min_usb_egress_Kbytes_per_second = context->usbStatistics.usb_egress_Kbytes_per_second;
		if(context->usbStatistics.usb_egress_Kbytes_per_second > context->usbStatistics.max_usb_egress_Kbytes_per_second)
			context->usbStatistics.max_usb_egress_Kbytes_per_second = context->usbStatistics.usb_egress_Kbytes_per_second;

		//Ingress
		usb_ingress_Kbytes_processed = (unsigned long)(context->usbStatistics.total_bytes_sent_usb_ingress/1000);
		context->usbStatistics.usb_ingress_Kbytes_per_second = (float)((float)usb_ingress_Kbytes_processed/60);


		if(context->usbStatistics.moving_average_usb_ingress == 0)
		{
			context->usbStatistics.moving_average_usb_ingress = context->usbStatistics.usb_ingress_Kbytes_per_second;
			//corrib_syslog(LOG_DEBUG,"%s:resetting avg=%f   \t",__func__,context->usbStatistics.moving_average_video_head1);
		}
		context->usbStatistics.moving_average_usb_ingress = hw_manager_get_rolling_average(context->usbStatistics.moving_average_usb_ingress,context->usbStatistics.usb_ingress_Kbytes_per_second,2);
		if(context->usbStatistics.usb_ingress_Kbytes_per_second < context->usbStatistics.min_usb_ingress_Kbytes_per_second )
			context->usbStatistics.min_usb_ingress_Kbytes_per_second = context->usbStatistics.usb_ingress_Kbytes_per_second;
		if(context->usbStatistics.usb_ingress_Kbytes_per_second > context->usbStatistics.max_usb_ingress_Kbytes_per_second)
			context->usbStatistics.max_usb_ingress_Kbytes_per_second = context->usbStatistics.usb_ingress_Kbytes_per_second;

		//common
		statistcs_send_json_usb_fact_object(&(context->usbStatistics));
		context->usbStatistics.previous_time_usb = current_time;
		context->usbStatistics.total_bytes_sent_usb_ingress=0;
		context->usbStatistics.total_bytes_sent_usb_egress=0;
	}
}

#ifdef ARPM
typedef struct {
	sequenceData sequence_data;
	uint8_t* usb_payload;
}TUsbPacket;
#endif

BOOL up_write_release_usb_command ( upContext* up_context, hwManagerContext* hm_context, upCommandResult cmdResult, int fd )
{
	BOOL result = false;
	char* data_ptr = NULL;
	int fdWritten = 0;
	int dataLength = 0;
	if ( cmdResult.cmd != NULL && up_context != NULL ) {
		data_ptr = (char *)cmdResult.cmd->data - USB_FD_HEADER_DATA_OFFSET;
		dataLength = cmdResult.cmd->dataLength + sizeof ( sequenceData );

#ifdef CONFIG_USB_BLACKBOX_USBR_ENABLE_PROFILING
		if (cmdResult.sequence_data != NULL) {
			/*
				This is the right place to add timestamps APP->DRIVER direction
			*/
			if ((cmdResult.sequence_data->type == USBSQT_URB_COMPLETION_DATA) &&
				// cmdResult.sequence_data->address it timestamps all bulk requests to any endpoint
				((cmdResult.sequence_data->attributes & 0x03) == USB_ENDPOINT_XFER_BULK) &&
				(cmdResult.sequence_data->maxPacketSize > 0))
			{
				// TIMESTAMP #11
				usbr_profiling_timestamp(cmdResult.cmd->data, cmdResult.cmd->dataLength, 11);
			}
		}
#endif

		fdWritten = write(fd, data_ptr, dataLength);
		if (fdWritten == dataLength) {
			result = true;
			if(hm_context) {
				hm_context->usbStatistics.total_bytes_sent_usb_ingress += dataLength;
				up_generate_statistics(hm_context);
			}
		}
		else {
			corrib_syslog ( LOG_ERR, "%s: Failed to write USBR data to the kernel, fatal usbr error\n", __func__);
		}
#ifdef MEMORY_LEAK_DEBUG            
                corrib_syslog ( LOG_DEBUG, "%s(): Freeing cmdResult.cmd\n", __func__);
#endif
		usb_command_free(cmdResult.cmd);
		if ( cmdResult.sequence_data != NULL ) {
#ifdef MEMORY_LEAK_DEBUG            
            corrib_syslog ( LOG_DEBUG, "%s(): Freeing cmdResult.sequence_data 1\n", __func__);
#endif
			xfree(cmdResult.sequence_data, __func__);
		}
	}
	else if ( cmdResult.sequence_data != NULL ) {
		data_ptr = (char*) cmdResult.sequence_data;
		dataLength = sizeof(*cmdResult.sequence_data);

		fdWritten = write(fd, data_ptr, dataLength);
		if (fdWritten == dataLength) {
			result = true;
			if (hm_context) {
				hm_context->usbStatistics.total_bytes_sent_usb_egress += dataLength;
				up_generate_statistics(hm_context);
			}
		}
		else {
			
			corrib_syslog ( LOG_ERR, "%s: Failed to write USBR data to the kernel, fatal usbr error", __func__);
		}
#ifdef MEMORY_LEAK_DEBUG            
                corrib_syslog ( LOG_DEBUG, "%s(): Freeing cmdResult.sequence_data 2\n", __func__);
#endif
		xfree(cmdResult.sequence_data, __func__);
	}
	return result;

}

BOOL up_release_usb_command(upContext* up_context, hwManagerContext* hm_context, upCommandResult cmdResult)
{
	BOOL result = false;
	char* data_ptr = NULL;
	int dataLength = 0;

	if ( cmdResult.cmd != NULL && up_context != NULL ) {
		data_ptr = (char *)cmdResult.cmd->data - USB_FD_HEADER_DATA_OFFSET;
		dataLength = cmdResult.cmd->dataLength + sizeof(*cmdResult.sequence_data);
		usb_command_free(cmdResult.cmd);
		if ( cmdResult.sequence_data != NULL ) {
#ifdef MEMORY_LEAK_DEBUG            
                corrib_syslog ( LOG_DEBUG, "%s(): Freeing cmdResult,sequence_data 3\n", __func__);
#endif
			xfree(cmdResult.sequence_data, __func__);
		}
		result = true;
	}
	else if ( cmdResult.sequence_data ) {
		data_ptr = (char*) cmdResult.sequence_data;
		dataLength = sizeof(*cmdResult.sequence_data);
#ifdef MEMORY_LEAK_DEBUG            
                corrib_syslog ( LOG_DEBUG, "%s(): Freeing cmdResult,sequence_data 4\n", __func__);
#endif
		xfree(cmdResult.sequence_data, __func__);
		result = true;
	}
	return result;
}


