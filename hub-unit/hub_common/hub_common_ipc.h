#ifndef HUB_COMMON_IPC_H
#define HUB_COMMON_IPC_H

#include "hub_common.h"

#include <mqueue.h>
#include <semaphore.h>

#define DOOR_MANAGER_INBOX_QUEUE_NAME "/mq_doors_door_manager_inbox"
#define INTERCOM_SERVER_INBOX_QUEUE_NAME "/mq_doors_intercom_server_inbox"
#define DATABASE_SERVICE_INBOX_QUEUE_NAME "/mq_doors_database_service_inbox"

#define MQ_MSG_SIZE_MAX (sizeof(DoorPacket_t) + DOOR_DATA_BYTES_SMALL)
#define MQ_DB_MSG_SIZE_MAX (sizeof(DoorPacket_t))
#define MQ_MSG_COUNT_MAX (10)

#define DOOR_STATES_SHM_NAME "DOORS_DOOR_STATES_SHM"
#define INTERCOM_STATES_SHM_NAME "DOORS_INTERCOM_STATES_SHM"
#define HUB_LOG_SHM_NAME "DOORS_HUB_LOG_SHM"

#define DOOR_STATES_SEM_NAME "DOORS_DOOR_STATES_SEM"
#define INTERCOM_STATES_SEM_NAME "DOORS_INTERCOM_STATES_SEM"
#define HUB_LOG_SEM_NAME "DOORS_HUB_LOG_SEM"

typedef struct HubHandles
{
    mqd_t door_manager_inbox_handle;
    mqd_t intercom_server_inbox_handle;
    mqd_t database_service_inbox_handle;

    sem_t *door_states_sem_ptr;
    void *door_states_shm_ptr;

    sem_t *intercom_states_sem_ptr;
    void *intercom_states_shm_ptr;
} HubHandles_t;

void ipc_send_packet_copy_to_db(DoorPacket_t *source);

bool ipc_init_hub_log_ptrs(bool init_shm);
void ipc_deinit_hub_log_ptrs(void);

bool ipc_init_door_states_ptrs(bool init_shm);

bool ipc_init_intercom_states_ptrs(bool init_shm);

bool ipc_init_inbox_handles(void);
void ipc_deinit_inbox_handles(void);

HubIntercomStates_t *ipc_acquire_intercom_states_ptr(bool blocking);
void ipc_release_intercom_states_ptr(void);

HubDoorStates_t *ipc_acquire_door_states_ptr(bool blocking);
void ipc_release_door_states_ptr(void);

HubLogRing_t *ipc_acquire_hub_log_ptr(void);
void ipc_release_hub_log_ptr(void);

const HubHandles_t* ipc_get_hub_handles_ptr(void);

#endif
