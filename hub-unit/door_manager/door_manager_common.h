#ifndef DOOR_MANAGER_COMMON_H
#define DOOR_MANAGER_COMMON_H

#include "packet_defs.h"
#include "packet_utils.h"
#include "hub_common.h"
#include "i2c_register_defs.h"

#include "door_manager_i2c.h"
#include "door_manager_ipc.h"

#define TARGET_ADDR_OFFSET (I2C_MIN_ADDRESS)
#define TARGET_ADDR_MAX_COUNT (I2C_MAX_ADDRESS - I2C_MIN_ADDRESS)

extern HubQueue_t *doors_to_clients_queue;

void common_init(void);
void common_terminate(int ret);

#endif
