#ifndef INTERCOM_SERVER_IPC_H
#define INTERCOM_SERVER_IPC_H

#include "intercom_server_common.h"

void* ipc_in_task(void *arg);
void* ipc_out_task(void *arg);
HubClientStates_t *ipc_acquire_intercom_states_ptr(void);
void ipc_release_intercom_states_ptr(void);
HubDoorStates_t *ipc_acquire_door_states_ptr(void);
void ipc_release_door_states_ptr(void);
void ipc_forward_client_to_door_request(DoorPacket_t *request);
void ipc_init(void);

#endif
