#ifndef WEB_SERVER_IPC_H
#define WEB_SERVER_IPC_H

#include <string.h>

#include "hub_common_ipc.h"

HubClientStates_t *ipc_acquire_intercom_states_ptr(void);
void ipc_release_intercom_states_ptr(void);
HubDoorStates_t *ipc_acquire_door_states_ptr(void);
void ipc_release_door_states_ptr(void);
void ipc_init(void);
void ipc_deinit(void);

#endif
