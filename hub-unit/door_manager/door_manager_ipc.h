#ifndef DOOR_MANAGER_IPC_H
#define DOOR_MANAGER_IPC_H

#include <string.h>

#include "door_manager_common.h"
#include "door_manager_i2c.h"

void ipc_terminate(void);
void* ipc_task(void *arg);
void ipc_init(void);

#endif
