#ifndef DOOR_MANAGER_IPC_H
#define DOOR_MANAGER_IPC_H

#include <string.h>

#include "door_manager_common.h"
#include "door_manager_i2c.h"

void* ipc_in_task(void *arg);
void* ipc_out_task(void *arg);
HubDoorStates_t *ipc_acquire_door_states_ptr(void);
void ipc_release_door_states_ptr(void);
void ipc_init(void);
void ipc_deinit(void);

#endif
