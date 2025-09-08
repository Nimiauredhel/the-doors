#ifndef HUB_COMMON_IPC_H
#define HUB_COMMON_IPC_H

#include "hub_common.h"

typedef struct HubHandles
{
    mqd_t door_manager_inbox_handle;
    mqd_t intercom_server_inbox_handle;

    sem_t *door_states_sem_ptr;
    void *door_states_shm_ptr;

    sem_t *intercom_states_sem_ptr;
    void *intercom_states_shm_ptr;
} HubHandles_t;

bool ipc_init_door_states_ptrs(bool init_shm);
bool ipc_init_intercom_states_ptrs(bool init_shm);
bool ipc_init_inbox_handles(void);
HubClientStates_t *ipc_acquire_intercom_states_ptr(void);
void ipc_release_intercom_states_ptr(void);
HubDoorStates_t *ipc_acquire_door_states_ptr(void);
void ipc_release_door_states_ptr(void);
const HubHandles_t* ipc_get_hub_handles_ptr(void);

#endif
