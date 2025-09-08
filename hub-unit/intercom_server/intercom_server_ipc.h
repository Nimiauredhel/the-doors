#ifndef INTERCOM_SERVER_IPC_H
#define INTERCOM_SERVER_IPC_H

#include "intercom_server_common.h"
#include "hub_common_ipc.h"

void* ipc_in_task(void *arg);
void* ipc_out_task(void *arg);
void ipc_init(void);
void ipc_deinit(void);

#endif
