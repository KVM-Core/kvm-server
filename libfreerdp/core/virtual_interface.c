/*
 * Brian Quinn
 * Copyright: Cloudium Systems 2015
 */

#include "virtual_interface.h"
#include <freerdp/utils/sh_logger.h>
#include <freerdp/core_event.h>

BOOL virtual_interface_fds_set(virtualInterface* virtual_interface, fd_set* rfds_set, virtualDevice** virtual_device)
{
	BOOL status = false;
	for( int index = 0; index < virtual_interface->size; index++) {
		if ( FD_ISSET ( virtual_interface->devices[index].fd, rfds_set ) ) {
#ifdef DEBUG_ENABLED
			corrib_syslog(LOG_DEBUG,"** virtual_interface_fds_set virtual_interface->device %p fd %d id %d**\n", &virtual_interface->devices[index], virtual_interface->devices[index].fd, virtual_interface->devices[index].device_id);
#endif
			*virtual_device = &virtual_interface->devices[index];
			status = true;
			break;
		}
	}
	return status;
}

virtualDevice* vitual_interface_get_device(virtualInterface* virtual_interface, int device_id)
{
	virtualDevice* virtual_device = NULL;
	for( int index = 0; index < virtual_interface->size; index ++ ) {
		if ( virtual_interface->devices[index].device_id == device_id ) {
			virtual_device = &virtual_interface->devices[index];
			break;
		}
	}
	return virtual_device;
}

void virtual_interface_process_event(upContext * up_context, hwManagerContext * hm_context, eqEvent* event)
{
	EventUsbCommandAvailable* usb_event = (EventUsbCommandAvailable*) event;
	if ( usb_event != NULL ) {
		switch ( usb_event->sequence_data->type ) {
			//ARPM!!
			case USBSQT_RELEASE_COMMAND: // For releasing memory associated with the command
			{
#ifdef DEBUG_ENABLED
				fprintf (stderr, "%s(): USBSQT_RELEASE_COMMAND\n", __func__);
#endif
				if ( !up_release_usb_command(up_context, hm_context, (upCommandResult){ usb_event->sequence_data, (USB_COMMAND*)usb_event->cmd }) ) {
					corrib_syslog ( LOG_ERR, "%s: We failed to release some USBR data for event %u, this could cause a memory leak\n", __func__,usb_event->sequence_data->type);
				}
				break;
			}
			case USBSQT_REQUEST_RESOURCE: // when a device is initially plugged in, it normally sends this command
			{
				int index;
				BOOL command_released = false;
				virtualInterface* virtual_interface = &hm_context->virtual_interface;

				corrib_syslog(LOG_ERR, "%s(): USBSQT_REQUEST_RESOURCE begin\n", __func__);

				for ( index = 0; index < virtual_interface->size; index++ ) {
					if (!virtual_interface->devices[index].device_id) {
						virtual_interface->devices[index].device_id = usb_event->device_id;
corrib_syslog(LOG_DEBUG, "%s(): suspected area begin\n", __func__);
						if ( ! up_write_release_usb_command( up_context, hm_context, (upCommandResult) { usb_event->sequence_data, (USB_COMMAND*)usb_event->cmd }, virtual_interface->devices[index].fd ) ) {
							virtual_interface->devices[index].device_id = 0;
							corrib_syslog ( LOG_ERR, "%s Failed to release or write USBR data to the kernel for event %u\n", __func__,usb_event->sequence_data->type);
							break;
						}
corrib_syslog(LOG_DEBUG, "%s(): suspected area end\n", __func__);
						virtual_interface->devices[index].device_id = usb_event->device_id;
						command_released = true;
						break;
					}
				}
				if ( virtual_interface->devices[index].device_id != usb_event->device_id ) {
					/* create an end event to stop the hardware manager */
					corrib_syslog ( LOG_ERR, "%s: Could not find the required virtual device ID for event %u, fatal usbr error.\n", __func__,usb_event->sequence_data->type);
					/* ARPM: Error command creation */
					EventUsbCommandAvailable * usb_command_available_event = event_usb_command_available_new(usb_event->sequence_data, usb_event->device_id, usb_event->cmd);
					if (usb_command_available_event) {
						usb_command_available_event->sequence_data->result = USBSQR_FAILURE; /* ARPM:  Where is this event released? */
						eq_push(hm_context->hm_cm_queue, (eqEvent *)usb_command_available_event);
					}
				}
				else if ( !command_released ) {
					usb_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
					if ( !up_release_usb_command(up_context, hm_context, (upCommandResult){ usb_event->sequence_data, (USB_COMMAND*)usb_event->cmd }) ) {
						corrib_syslog ( LOG_ERR, "%s: We failed to release some USBR data for event %u, this could cause a memory leak\n", __func__,usb_event->sequence_data->type);
					}
					command_released = true;
				}
				corrib_syslog(LOG_ERR, "%s(): USBSQT_REQUEST_RESOURCE end\n", __func__);
				break;
			}
			case USBSQT_RELEASE_RESOURCE:  //release net2272 resources
			case USBSQT_CONFIGURE_RESOURCE: // consume a net2272 resource
			case USBSQT_STANDARD_IO_CONTROL:  //only used when a driver wants to reset the remote device
			case USBSQT_INTERNAL_IO_CONTROL:  // not used
			case USBSQT_TRANSFER_IN_REQUEST:  //follows config of a resource
			case USBSQT_TRANSFER_OUT_REQUEST: //follows config of a resource
			case USBSQT_IO_CONTROL_COMPLETION: //only used when a driver wants to reset the remote device - acknowledge of the IO Control Request
			case USBSQT_URB_COMPLETION_NO_DATA: // response for commands that have no associated data
			case USBSQT_URB_COMPLETION_DATA: // response for commands that have associated data
			{
				virtualDevice* virtual_device = vitual_interface_get_device(&hm_context->virtual_interface, usb_event->device_id);
				if ( virtual_device != NULL ) {
#ifdef DEBUG_ENABLED
					fprintf (stderr, "%s(): URB_COMPLETION[_NO_DATA]\n", __func__);
#endif
					if ( !up_write_release_usb_command( up_context, hm_context, (upCommandResult) { usb_event->sequence_data, (USB_COMMAND *) usb_event->cmd }, virtual_device->fd ) ) {
						corrib_syslog ( LOG_ERR, "%s Failed to release or write USBR data to the kernel for event %u\n",__func__,usb_event->sequence_data->type);
					}
				}
				else {
					usb_event->sequence_data->type = USBSQT_RELEASE_COMMAND;
					if ( !up_release_usb_command (up_context, hm_context, (upCommandResult) {usb_event->sequence_data, (USB_COMMAND*)usb_event->cmd}) ) {
						corrib_syslog ( LOG_ERR, "%s: We failed to release some USBR data for event %u, this could cause a memory leak\n", __func__,usb_event->sequence_data->type);
					}
				}
				break;
			}
			default:
			{
				corrib_syslog ( LOG_ERR, "%s(): Unknown _SEQUENCE_TYPE [0x%02x]\n",  __func__, usb_event->type );
				break;
			}
		}
		/*
		 * ARPM: At this stage usb_event->cmd and usb_event->sequence_data should be released but we still need to release data related to the event itself.
		 */
	}
}

void virtual_interface_reset_devices(virtualInterface* virtual_interface)
{
	corrib_syslog(LOG_DEBUG,"%s(): start\n", __func__);
	for ( int index = 0; index < virtual_interface->size; index++ ) {
		if ( virtual_interface->devices[index].fd > 0 ) {
			sequenceData* sequence_data = xzalloc(sizeof(sequenceData),__func__);
			sequence_data->type = USBSQT_RELEASE_RESOURCE;
			sequence_data->address = 0x80;
			if ( !up_write_release_usb_command(NULL, NULL, (upCommandResult) { sequence_data, NULL }, virtual_interface->devices[index].fd) ) {			
			corrib_syslog(LOG_ERR,"%s Failed to release or write USBR data to the kernel for virtualinterface index %u\n ",__func__,index);
			}
		}
	}
	corrib_syslog(LOG_DEBUG,"%s(): end\n", __func__);
	return;
}

BOOL virtual_interface_init_devices(virtualInterface* virtual_interface)
{
	int flags;
	BOOL status = true;
	virtual_interface->size = 0;
	char virtDeviceNameZero [] = {"/dev/virtg0"};
	char virtDeviceNameOne [] = {"/dev/virtg1"};

#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG,"%s(): start\n", __func__);
#endif
	/*
	 * ARPM: TODO: Change this to a "for" structure!
	 */
	/* Open virtual device 0 */
	if ((virtual_interface->devices[virtual_interface->size].fd = open(virtDeviceNameZero, O_RDWR)) == -1) {
		perror(virtDeviceNameZero);
		corrib_syslog(LOG_ERR,"%s(): Failed to open virtual device %s\n",__func__, virtDeviceNameZero);
		status = false;
	}
	else {
		flags = fcntl(virtual_interface->devices[virtual_interface->size].fd, F_GETFL, 0);
		fcntl(virtual_interface->devices[virtual_interface->size].fd, F_SETFL, flags | O_NONBLOCK);
		virtual_interface->size ++;
		corrib_syslog (LOG_INFO, "%s(): Virtual interface [%s] opened", __func__, virtDeviceNameZero);
	}
	/* Open virtual device 1 */
	if ((virtual_interface->devices[virtual_interface->size].fd = open(virtDeviceNameOne, O_RDWR)) == -1)
	{
		perror(virtDeviceNameOne);
		corrib_syslog(LOG_ERR,"%s(): Failed to open virtual device %s\n",__func__, virtDeviceNameOne);
		status = status || false;
	}
	else
	{
		flags = fcntl(virtual_interface->devices[virtual_interface->size].fd, F_GETFL, 0);
		fcntl(virtual_interface->devices[virtual_interface->size].fd, F_SETFL, flags | O_NONBLOCK);
		virtual_interface->size ++;
		corrib_syslog (LOG_INFO, "%s(): Virtual interface [%s] opened", __func__, virtDeviceNameOne);
	}
#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG,"%s(): end\n", __func__);
#endif
	return status;
}

void virtual_interface_deinit(virtualInterface* virtual_interface)
{
	int index;
#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG,"%s(): start\n", __func__);
#endif
	for(index = 0; index < virtual_interface->size; index++) {
		if (virtual_interface->devices[index].fd) {
			close(virtual_interface->devices[index].fd);
			break;
		}
	}
#ifdef DEBUG_ENABLED
	corrib_syslog(LOG_DEBUG,"%s(): end\n", __func__);
#endif
}
