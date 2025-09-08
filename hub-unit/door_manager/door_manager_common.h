#ifndef DOOR_MANAGER_COMMON_H
#define DOOR_MANAGER_COMMON_H

#include "packet_defs.h"
#include "packet_utils.h"
#include "hub_common.h"
#include "i2c_register_defs.h"

#define TARGET_ADDR_OFFSET (I2C_MIN_ADDRESS)
#define TARGET_ADDR_MAX_COUNT (I2C_MAX_ADDRESS - I2C_MIN_ADDRESS)

extern HubQueue_t *doors_to_clients_queue;

void door_manager_update_door_list_txt(HubDoorStates_t *door_states_ptr);
void door_manager_init(void);
void door_manager_deinit(void);

#endif
