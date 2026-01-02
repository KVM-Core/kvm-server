#ifndef __VIRTUAL_API_INTERFACE_H
#define __VIRTUAL_API_INTERFACE_H

#include <freerdp/hardware_manager.h>
#include "usb_packetizer.h"

//#define DEBUG_ENABLED 1

boolean virtual_interface_fds_set(virtualInterface* virtual_interface, fd_set* rfds_set, virtualDevice** virtual_device);
virtualDevice* vitual_interface_get_device(virtualInterface* virtual_interface, int device_id);
void virtual_interface_process_event(upContext * up_context, hwManagerContext * hm_context, eqEvent* event);
void virtual_interface_reset_devices(virtualInterface* virtual_interface);
boolean virtual_interface_init_devices(virtualInterface* virtual_interface);
void virtual_interface_deinit(virtualInterface* virtual_interface);

#endif //__VIRTUAL_API_INTERFACE_H
