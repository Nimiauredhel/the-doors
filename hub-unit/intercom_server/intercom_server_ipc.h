#ifndef INTERCOM_SERVER_IPC_H
#define INTERCOM_SERVER_IPC_H

#include "intercom_server_common.h"

void forward_client_to_door_request(DoorPacket_t *request);
void ipc_init(void);
void* ipc_in_task(void *arg);
void* ipc_out_task(void *arg);

#endif
