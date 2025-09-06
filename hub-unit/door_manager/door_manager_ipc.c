#include "door_manager_ipc.h"

static mqd_t ipc_inbox_handle;
static mqd_t ipc_outbox_handle;

static sem_t *ipc_door_states_sem_ptr;
static void *ipc_door_states_shm_ptr;

static sem_t *ipc_intercom_states_sem_ptr;
static void *ipc_intercom_states_shm_ptr;

static void ipc_init_door_states_shm(void)
{
    /// TODO: most of this section is duplicated across modules and should be extracted

    ipc_door_states_sem_ptr = sem_open(DOOR_STATES_SEM_NAME, O_CREAT | O_RDWR, 0666, 0);

    if (ipc_door_states_sem_ptr == NULL)
    {
        log_append("Failed to open door states semaphore.");
        exit(EXIT_FAILURE);
    }

    log_append("Opened door states semaphore.");

    if (0 > sem_init(ipc_door_states_sem_ptr, 1, 0))
    {
        log_append("Failed to initialize door states semaphore.");
        exit(EXIT_FAILURE);
    }

    log_append("Initialized door states semaphore.");

    sem_post(ipc_door_states_sem_ptr);

    int shm_fd;

    shm_fd = shm_open(DOOR_STATES_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd <= 0)
    {
        log_append("Failed to open door states shm.");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, sizeof(HubDoorStates_t));
    ipc_door_states_shm_ptr = mmap(0, sizeof(HubDoorStates_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ipc_door_states_shm_ptr == NULL)
    {
        log_append("Failed to map door states shm.");
        exit(EXIT_FAILURE);
    }

    log_append("Mapped door states shm.");

    explicit_bzero(ipc_door_states_shm_ptr, sizeof(HubDoorStates_t));
}

static void ipc_link_intercom_states_shm(void)
{
    /// TODO: most of this section is duplicated across modules and should be extracted

    ipc_intercom_states_sem_ptr = sem_open(CLIENT_STATES_SEM_NAME, O_CREAT | O_RDWR, 0666, 0);

    if (ipc_intercom_states_sem_ptr == NULL)
    {
        log_append("Failed to open intercom states semaphore.");
        exit(EXIT_FAILURE);
    }

    log_append("Opened intercom states semaphore.");

    int shm_fd;

    shm_fd = shm_open(CLIENT_STATES_SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd <= 0)
    {
        log_append("Failed to open intercom states shm.");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, sizeof(HubClientStates_t));
    ipc_intercom_states_shm_ptr = mmap(0, sizeof(HubClientStates_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ipc_intercom_states_shm_ptr == NULL)
    {
        log_append("Failed to map intercom states shm.");
        exit(EXIT_FAILURE);
    }

    log_append("Mapped intercom states shm.");
}

static void ipc_in_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    bytes_transmitted = mq_receive(ipc_inbox_handle, msg_buff, sizeof(msg_buff), NULL);

    if (bytes_transmitted < 0)
    {
        snprintf(log_buff, sizeof(log_buff), "Failed to receive from inbox: %s", strerror(errno));
        log_append(log_buff);
        sleep(1);
    }
    else
    {
        i2c_forward_request((DoorPacket_t *)msg_buff);
    }
}

static void ipc_out_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    if (hub_queue_dequeue(doors_to_clients_queue, (DoorPacket_t *)&msg_buff) >= 0)
    {
        log_append("Forwarding from internal queue to outbox.");

        for(;;)
        {
            bytes_transmitted = mq_send(ipc_outbox_handle, msg_buff, sizeof(msg_buff), 0);

            if (bytes_transmitted >= 0) break;

            snprintf(log_buff, sizeof(log_buff), "Failed forwarding to outbox: %s", strerror(errno));
            log_append(log_buff);
            sleep(1);
        }
    }
    else
    {
        sleep(1);
    }
}

void* ipc_in_task(void *arg)
{
    for(;;) ipc_in_loop();
    return NULL;
}

void* ipc_out_task(void *arg)
{
    for(;;) ipc_out_loop();
    return NULL;
}

HubClientStates_t *ipc_acquire_intercom_states_ptr(void)
{
    sem_wait(ipc_intercom_states_sem_ptr);
    return (HubClientStates_t *)ipc_intercom_states_sem_ptr;
}

void ipc_release_intercom_states_ptr(void)
{
    sem_post(ipc_intercom_states_sem_ptr);
}

HubDoorStates_t *ipc_acquire_door_states_ptr(void)
{
    sem_wait(ipc_door_states_sem_ptr);
    return (HubDoorStates_t *)ipc_door_states_sem_ptr;
}

void ipc_release_door_states_ptr(void)
{
    sem_post(ipc_door_states_sem_ptr);
}

void ipc_init(void)
{
    log_append("Initializing IPC");

    ipc_outbox_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_WRONLY);

    if (ipc_outbox_handle < 0)
    {
        perror("Failed to open outbox queue");
        log_append("Failed to open outbox queue");
        exit(EXIT_FAILURE);
    }

    log_append("Opened outbox queue");

    ipc_inbox_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_RDONLY);

    if (ipc_outbox_handle < 0)
    {
        perror("Failed to open inbox queue");
        log_append("Failed to open inbox queue");
        exit(EXIT_FAILURE);
    }

    log_append("Opened inbox queue");

    ipc_init_door_states_shm();
    ipc_link_intercom_states_shm();

    log_append("IPC Initialization Complete");
}

void ipc_deinit(void)
{
    mq_close(ipc_inbox_handle);
    mq_close(ipc_outbox_handle);
}
