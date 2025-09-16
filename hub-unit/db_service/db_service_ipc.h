#ifndef DB_SERVICE_IPC_H
#define DB_SERVICE_IPC_H

#include "hub_common_ipc.h"

void* ipc_in_task(void *arg);
void ipc_init(void);
void ipc_deinit(void);

#endif
