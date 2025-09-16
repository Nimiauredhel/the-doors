#include "intercom_server_ipc.h"
#include "intercom_server_listener.h"
#include "intercom_server_common.h"

static const struct timespec loop_delay =
{
    .tv_nsec = 500000000,
    .tv_sec = 0,
};
static const struct timespec mq_timeout =
{
    .tv_nsec = 0,
    .tv_sec = 1,
};

static const HubHandles_t *hub_handles_ptr = NULL;

/**
 * @brief Reads messages from the 'intercom server inbox' POSIX queue,
 * and forwards them for further processing (which typically means
 * forwarding them to a particular Intercom Unit on the local network).
 * A generous timeout duration is given to allow the calling thread
 * to terminate if necessary.
 **/
static void ipc_in_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    nanosleep(&loop_delay, NULL);

    bytes_transmitted = mq_timedreceive(hub_handles_ptr->intercom_server_inbox_handle, msg_buff, sizeof(msg_buff), NULL, &mq_timeout);

    if (bytes_transmitted <= 0)
    {
        if (errno == ETIMEDOUT || errno == EAGAIN || errno == EWOULDBLOCK)
        {
        }
        else
        {
            snprintf(log_buff, sizeof(log_buff), "Failed to receive from inbox: %s", strerror(errno));
            log_append(log_buff);
        }

        /**
         * intentionally only sleeps when nothing was dequeued,
         * so that sequential inputs will be processed as fast as possible.
         **/
        nanosleep(&loop_delay, NULL);
    }
    else
    {
        forward_door_to_client_request((DoorPacket_t *)&msg_buff);
    }
}

/**
 * @brief Reads messages from the internal 'clients to doors' queue,
 * and forwards them to the 'door manager inbox' POSIX queue.
 **/
static void ipc_out_loop(void)
{
    /// TODO: this function is nearly duplicated in 'door_manager_ipc.c', consider extracting.
    
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    if (hub_queue_dequeue(clients_to_doors_queue, (DoorPacket_t *)&msg_buff) >= 0)
    {
        log_append("Forwarding from internal queue to outbox.");

        /**
         * this is a 'retry' loop - it breaks on success and reports on failures,
         * which is why the common exit condition is currently inside and not on top.
         * TODO: consider a retry counter or otherwise a more thought through error handling
         * TODO: possibly mark message priority and consider some messages droppable
         **/
        while(!should_terminate)
        {
            bytes_transmitted = mq_timedsend(hub_handles_ptr->door_manager_inbox_handle, msg_buff, sizeof(msg_buff), 0, &mq_timeout);

            if (bytes_transmitted >= 0) break;

            snprintf(log_buff, sizeof(log_buff), "Failed forwarding from internal queue to door manager inbox: %s", strerror(errno));
            log_append(log_buff);
            nanosleep(&loop_delay, NULL);
        }

        // send copy to db
        while(!should_terminate)
        {
            bytes_transmitted = mq_timedsend(hub_handles_ptr->db_service_inbox_handle, msg_buff, sizeof(msg_buff), 0, &mq_timeout);

            if (bytes_transmitted >= 0) break;

            snprintf(log_buff, sizeof(log_buff), "Failed forwarding to DB inbox: %s", strerror(errno));
            log_append(log_buff);
            nanosleep(&loop_delay, NULL);
        }
    }
    else
    {
        /**
         * intentionally only sleeps when nothing was dequeued,
         * so that sequential inputs will be processed as fast as possible.
         **/
        nanosleep(&loop_delay, NULL);
    }
}

void* ipc_in_task(void *arg)
{
    /// suppresses the 'unused variable' warning
    (void)arg;

    log_append("Starting IPC Input Task.");

    while(!should_terminate)
    {
        ipc_in_loop();
    }

    log_append("Ending IPC Input Task.");

    return NULL;
}

void* ipc_out_task(void *arg)
{
    /// suppresses the 'unused variable' warning
    (void)arg;

    log_append("Starting IPC Output Task.");

    while(!should_terminate)
    {
        ipc_out_loop();
    }

    log_append("Ending IPC Output Task.");

    return NULL;
}

void ipc_init(void)
{
    log_append("Initializing IPC");

    bool initialized = ipc_init_inbox_handles()
                    && ipc_init_door_states_ptrs(false)
                    && ipc_init_intercom_states_ptrs(true)
                    && (NULL != (hub_handles_ptr = ipc_get_hub_handles_ptr()));

    if (initialized)
    {
        log_append("IPC Initialization Completed.");

        /// initial update/creation of intercom states txt
        HubClientStates_t *intercom_states_ptr = ipc_acquire_intercom_states_ptr();
        common_update_intercom_list_txt(intercom_states_ptr);
        ipc_release_intercom_states_ptr();
    }
    else
    {
        log_append("IPC Initialization Failed.");
        should_terminate = true;
    }
}

void ipc_deinit(void)
{
    ipc_deinit_inbox_handles();

    if (hub_handles_ptr != NULL)
    {
        sem_close(hub_handles_ptr->door_states_sem_ptr);
        sem_close(hub_handles_ptr->intercom_states_sem_ptr);
        hub_handles_ptr = NULL;
    }
}
