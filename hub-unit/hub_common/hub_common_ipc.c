#include "hub_common_ipc.h"
#include "hub_common.h"

static HubHandles_t hub_handles =
{
    .intercom_server_inbox_handle = -1,
    .door_manager_inbox_handle = -1,

    .door_states_shm_ptr = NULL,
    .door_states_sem_ptr = NULL,

    .intercom_states_sem_ptr = NULL,
    .intercom_states_shm_ptr = NULL,
};

static sem_t *hub_log_sem_ptr;
static void *hub_log_shm_ptr;

bool ipc_init_door_states_ptrs(bool init_shm)
{
    int shm_fd = 0;

    hub_handles.door_states_sem_ptr = NULL;
    hub_handles.door_states_sem_ptr = sem_open(DOOR_STATES_SEM_NAME, O_CREAT | O_RDWR, 0666, 0);

    if (hub_handles.door_states_sem_ptr == NULL)
    {
        log_append("Failed to open door states semaphore.");
        goto err;
    }

    if (init_shm)
    {
        if (0 > sem_init(hub_handles.door_states_sem_ptr, 1, 0))
        {
            log_append("Failed to initialize door states semaphore.");
            goto err;
        }

        log_append("Initialized door states semaphore.");
    }
    else
    {
        log_append("Opened door states semaphore.");
    }

    shm_fd = shm_open(DOOR_STATES_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd <= 0)
    {
        log_append("Failed to open door states shm.");
        goto err;
    }

    if (0 > ftruncate(shm_fd, sizeof(HubDoorStates_t)))
    {
        log_append("Failed to truncate door states shm.");
        goto err;
    }

    hub_handles.door_states_shm_ptr = mmap(0, sizeof(HubDoorStates_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (hub_handles.door_states_shm_ptr == NULL)
    {
        log_append("Failed to map door states shm.");
        goto err;
    }

    if (init_shm)
    {
        explicit_bzero(hub_handles.door_states_shm_ptr, sizeof(HubDoorStates_t));
        sem_post(hub_handles.door_states_sem_ptr);
        log_append("Initialized door states shm.");
    }
    else
    {
        log_append("Mapped door states shm.");
    }

    return true;

err:
    if (hub_handles.intercom_states_sem_ptr != NULL)
    {
        sem_close(hub_handles.intercom_states_sem_ptr);
        hub_handles.intercom_states_sem_ptr = NULL;
    }

    if (hub_handles.intercom_states_shm_ptr != NULL)
    {
        hub_handles.intercom_states_shm_ptr = NULL;
    }
    return false;
}

bool ipc_init_intercom_states_ptrs(bool init_shm)
{
    int shm_fd = 0;

    hub_handles.intercom_states_sem_ptr = NULL;
    hub_handles.intercom_states_sem_ptr = sem_open(CLIENT_STATES_SEM_NAME, O_CREAT | O_RDWR, 0666, 0);

    if (hub_handles.intercom_states_sem_ptr == NULL)
    {
        log_append("Failed to open intercom states semaphore.");
        goto err;
    }

    if (init_shm)
    {
        if (0 > sem_init(hub_handles.intercom_states_sem_ptr, 1, 0))
        {
            log_append("Failed to initialize intercom states semaphore.");
            goto err;
        }

        log_append("Initialized intercom states semaphore.");
    }
    else
    {
        log_append("Opened intercom states semaphore.");
    }

    shm_fd = shm_open(CLIENT_STATES_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd <= 0)
    {
        log_append("Failed to open intercom states shm.");
        goto err;
    }

    if (0 > ftruncate(shm_fd, sizeof(HubClientStates_t)))
    {
        log_append("Failed to truncate intercom states shm.");
        goto err;
    }

    hub_handles.intercom_states_shm_ptr = mmap(0, sizeof(HubClientStates_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (hub_handles.intercom_states_shm_ptr == NULL)
    {
        log_append("Failed to map intercom states shm.");
        goto err;
    }

    if (init_shm)
    {
        explicit_bzero(hub_handles.intercom_states_shm_ptr, sizeof(HubClientStates_t));
        sem_post(hub_handles.intercom_states_sem_ptr);
        log_append("Initialized intercom states shm.");
    }
    else
    {
        log_append("Mapped intercom states shm.");
    }

    return true;

err:
    if (hub_handles.intercom_states_sem_ptr != NULL)
    {
        sem_close(hub_handles.intercom_states_sem_ptr);
        hub_handles.intercom_states_sem_ptr = NULL;
    }
    if (hub_handles.intercom_states_shm_ptr != NULL)
    {
        hub_handles.intercom_states_shm_ptr = NULL;
    }
    return false;
}

bool ipc_init_inbox_handles(void)
{
    hub_handles.intercom_server_inbox_handle = -1;
    hub_handles.door_manager_inbox_handle = -1;

    hub_handles.intercom_server_inbox_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_RDWR);

    if (hub_handles.intercom_server_inbox_handle < 0)
    {
        perror("Failed to open intercom server inbox queue");
        log_append("Failed to open intercom server inbox queue");
        goto err;
    }

    log_append("Opened intercom server inbox queue");

    hub_handles.door_manager_inbox_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_RDWR);

    if (hub_handles.door_manager_inbox_handle < 0)
    {
        perror("Failed to open door manager inbox queue");
        log_append("Failed to door manager inbox queue");
        goto err;
    }

    log_append("Opened door manager inbox queue");

    return true;

err:
    if (hub_handles.intercom_server_inbox_handle >= 0)
    {
        mq_close(hub_handles.intercom_server_inbox_handle);
        hub_handles.intercom_server_inbox_handle = -1;
    }
    if (hub_handles.door_manager_inbox_handle >= 0)
    {
        mq_close(hub_handles.door_manager_inbox_handle);
        hub_handles.door_manager_inbox_handle = -1;
    }
    return false;
}

/// TODO: third near identical function - should genericise
bool ipc_init_hub_log_ptrs(bool init_shm)
{
    int shm_fd = 0;

    hub_log_sem_ptr = NULL;
    hub_log_sem_ptr = sem_open(HUB_LOG_SEM_NAME, O_CREAT | O_RDWR, 0666, 0);

    if (hub_log_sem_ptr == NULL)
    {
        log_append("Failed to open hub log semaphore.");
        goto err;
    }

    if (init_shm)
    {
        if (0 > sem_init(hub_log_sem_ptr, 1, 0))
        {
            log_append("Failed to initialize hub log semaphore.");
            goto err;
        }

        log_append("Initialized hub log semaphore.");
    }
    else
    {
        log_append("Opened hub log semaphore.");
    }

    shm_fd = shm_open(HUB_LOG_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd <= 0)
    {
        log_append("Failed to open hub log shm.");
        goto err;
    }

    if (0 > ftruncate(shm_fd, sizeof(HubLogRing_t)))
    {
        log_append("Failed to truncate hub log shm.");
        goto err;
    }

    hub_log_shm_ptr = mmap(0, sizeof(HubLogRing_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (hub_log_shm_ptr == NULL)
    {
        log_append("Failed to map hub log shm.");
        goto err;
    }

    if (init_shm)
    {
        explicit_bzero(hub_log_shm_ptr, sizeof(HubLogRing_t));
        sem_post(hub_log_sem_ptr);
        log_append("Initialized hub log shm.");
    }
    else
    {
        log_append("Mapped hub log shm.");
    }

    return true;

err:
    if (hub_log_sem_ptr != NULL)
    {
        sem_close(hub_log_sem_ptr);
        hub_log_sem_ptr = NULL;
    }

    if (hub_log_shm_ptr != NULL)
    {
        hub_log_shm_ptr = NULL;
    }
    return false;
}

HubClientStates_t *ipc_acquire_intercom_states_ptr(void)
{
    sem_wait(hub_handles.intercom_states_sem_ptr);
    return (HubClientStates_t *)hub_handles.intercom_states_shm_ptr;
}

void ipc_release_intercom_states_ptr(void)
{
    sem_post(hub_handles.intercom_states_sem_ptr);
}

HubDoorStates_t *ipc_acquire_door_states_ptr(void)
{
    sem_wait(hub_handles.door_states_sem_ptr);
    return (HubDoorStates_t *)hub_handles.door_states_sem_ptr;
}

void ipc_release_door_states_ptr(void)
{
    sem_post(hub_handles.door_states_sem_ptr);
}

HubLogRing_t *ipc_acquire_hub_log_ptr(void)
{
    sem_wait(hub_log_sem_ptr);
    return (HubLogRing_t *)hub_log_shm_ptr;
}

void ipc_release_hub_log_ptr(void)
{
    sem_post(hub_log_sem_ptr);
}

const HubHandles_t* ipc_get_hub_handles_ptr(void)
{
    return &hub_handles;
}
