#include "hub_common_ipc.h"
#include "hub_common.h"

static const struct mq_attr hub_mq_attributes =
{
    .mq_flags = 0,
    .mq_maxmsg = MQ_MSG_COUNT_MAX,
    .mq_msgsize = MQ_MSG_SIZE_MAX,
    .mq_curmsgs = 0,
};

static const struct timespec mq_timeout =
{
    .tv_nsec = 0,
    .tv_sec = 1,
};

static const struct timespec loop_delay =
{
    .tv_nsec = 500000000,
    .tv_sec = 0,
};

static HubHandles_t hub_handles =
{
    .intercom_server_inbox_handle = -1,
    .door_manager_inbox_handle = -1,
    .db_service_inbox_handle = -1,

    .door_states_shm_ptr = NULL,
    .door_states_sem_ptr = NULL,

    .intercom_states_sem_ptr = NULL,
    .intercom_states_shm_ptr = NULL,
};

static sem_t *hub_log_sem_ptr;
static void *hub_log_shm_ptr;

void ipc_send_packet_copy_to_db(DoorPacket_t *source)
{
    // send copy to db
    while(!should_terminate)
    {
        int bytes_transmitted = mq_timedsend(hub_handles.db_service_inbox_handle, (char *)source, sizeof(*source), 0, &mq_timeout);

        if (bytes_transmitted >= 0) break;

        nanosleep(&loop_delay, NULL);
    }
}

/// TODO: three near identical functions - should genericise
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

    if (shm_fd > 0) close(shm_fd);
    return true;

err:
    if (shm_fd > 0) close(shm_fd);
    ipc_deinit_hub_log_ptrs();
    return false;
}

void ipc_deinit_hub_log_ptrs(void)
{
    if (hub_log_sem_ptr != NULL)
    {
        sem_close(hub_log_sem_ptr);
        hub_log_sem_ptr = NULL;
    }

    if (hub_log_shm_ptr != NULL)
    {
        munmap(hub_log_shm_ptr, sizeof(HubLogRing_t));
        hub_log_shm_ptr = NULL;
    }
}

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
    // TODO: the repetitive logs can be compressed

    hub_handles.intercom_server_inbox_handle = -1;
    hub_handles.door_manager_inbox_handle = -1;
    hub_handles.db_service_inbox_handle = -1;

    // *** intercom server inbox ***
    hub_handles.intercom_server_inbox_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (hub_handles.intercom_server_inbox_handle < 0)
    {
        if (errno == EEXIST)
        {
            log_append("Intercom server inbox queue already exists.");

            hub_handles.intercom_server_inbox_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_RDWR);

            if (hub_handles.intercom_server_inbox_handle < 0)
            {
                log_append("Failed to open intercom server inbox queue.");
                goto err;
            }

            log_append("Opened intercom server inbox queue.");
        }
        else
        {
            log_append("Failed to open intercom server inbox queue.");
            goto err;
        }
    }
    else
    {
        log_append("Created intercom server inbox queue.");
    }

    // *** door manager inbox ***
    hub_handles.door_manager_inbox_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (hub_handles.door_manager_inbox_handle < 0)
    {
        if (errno == EEXIST)
        {
            log_append("Door manager inbox queue already exists.");
            hub_handles.door_manager_inbox_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_RDWR);

            if (hub_handles.door_manager_inbox_handle < 0)
            {
                log_append("Failed to open door manager inbox queue.");
                goto err;
            }

            log_append("Opened door manager inbox queue.");
        }
        else
        {
            log_append("Failed to open door manager inbox queue.");
            goto err;
        }
    }
    else
    {
        log_append("Created door manager inbox queue.");
    }

    // *** db service inbox ***
    hub_handles.db_service_inbox_handle = mq_open(DB_INBOX_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (hub_handles.db_service_inbox_handle < 0)
    {
        if (errno == EEXIST)
        {
            log_append("DB inbox queue already exists.");
            hub_handles.db_service_inbox_handle = mq_open(DB_INBOX_QUEUE_NAME, O_RDWR);

            if (hub_handles.db_service_inbox_handle < 0)
            {
                log_append("Failed to open DB inbox queue.");
                goto err;
            }

            log_append("Opened DB inbox queue.");
        }
        else
        {
            log_append("Failed to open DB inbox queue.");
            goto err;
        }
    }
    else
    {
        log_append("Created DB inbox queue.");
    }

    return true;

err:
    ipc_deinit_inbox_handles();
    return false;
}

void ipc_deinit_inbox_handles(void)
{
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

    if (hub_handles.db_service_inbox_handle >= 0)
    {
        mq_close(hub_handles.db_service_inbox_handle);
        hub_handles.db_service_inbox_handle = -1;
    }
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
