#include "door_manager_ipc.h"

static const useconds_t ipc_loop_delay_usec = 500000;
static const struct timespec mq_timeout =
{
    .tv_nsec = 0,
    .tv_sec = 1,
};

static const HubHandles_t *hub_handles_ptr = NULL;

/**
 * @brief Reads messages from the 'door manager inbox' POSIX queue,
 * and forwards them for further processing (which typically means
 * forwarding them to a particular Door Unit on the I2C bus).
 * A generous timeout duration is given to allow the calling thread
 * to terminate if necessary.
 **/
static void ipc_in_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    bytes_transmitted = mq_timedreceive(hub_handles_ptr->door_manager_inbox_handle, msg_buff, sizeof(msg_buff), NULL, &mq_timeout);

    if (bytes_transmitted < 0)
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
        usleep(ipc_loop_delay_usec);
    }
    else
    {
        i2c_forward_request((DoorPacket_t *)msg_buff);
    }
}

/**
 * @brief Reads messages from the internal 'doors to clients' queue,
 * and forwards them to the 'intercom server inbox' POSIX queue.
 **/
static void ipc_out_loop(void)
{
    /// TODO: this function is nearly duplicated in 'intercom_server_ipc.c', consider extracting.

    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    if (hub_queue_dequeue(doors_to_clients_queue, (DoorPacket_t *)&msg_buff) >= 0)
    {
        log_append("Forwarding from internal queue to intercom server inbox.");

        /**
         * this is a 'retry' loop - it breaks on success and reports on failures,
         * which is why the common exit condition is currently inside and not on top.
         * TODO: consider a retry counter or otherwise a more thought through error handling
         * TODO: possibly mark message priority and consider some messages droppable
         **/
        while(!should_terminate)
        {
            bytes_transmitted = mq_send(hub_handles_ptr->intercom_server_inbox_handle, msg_buff, sizeof(msg_buff), 0);

            if (bytes_transmitted >= 0) break;

            snprintf(log_buff, sizeof(log_buff), "Failed forwarding to intercom server inbox: %s", strerror(errno));
            log_append(log_buff);
            usleep(ipc_loop_delay_usec);
        }
    }
    else
    {
        /**
         * intentionally only sleeps when nothing was dequeued,
         * so that sequential inputs will be processed as fast as possible.
         **/
        usleep(ipc_loop_delay_usec);
    }
}

void* ipc_in_task(void *arg)
{
    /// suppresses the 'unused variable' warning
    (void)arg;

    while(!should_terminate) ipc_in_loop();
    return NULL;
}

void* ipc_out_task(void *arg)
{
    /// suppresses the 'unused variable' warning
    (void)arg;

    while(!should_terminate) ipc_out_loop();
    return NULL;
}

void ipc_init(void)
{
    log_append("Initializing IPC");

    bool initialized = ipc_init_inbox_handles()
                    && ipc_init_door_states_ptrs(true)
                    && ipc_init_intercom_states_ptrs(false)
                    && (NULL != (hub_handles_ptr = ipc_get_hub_handles_ptr()));

    if (initialized)
    {
        hub_handles_ptr = ipc_get_hub_handles_ptr();
        log_append("IPC Initialization Completed.");
    }
    else
    {
        log_append("IPC Initialization Failed.");
        should_terminate = true;
    }
}

void ipc_deinit(void)
{
    if (hub_handles_ptr != NULL)
    {
        mq_close(hub_handles_ptr->door_manager_inbox_handle);
        mq_close(hub_handles_ptr->intercom_server_inbox_handle);
        sem_close(hub_handles_ptr->door_states_sem_ptr);
        sem_close(hub_handles_ptr->intercom_states_sem_ptr);
        hub_handles_ptr = NULL;
    }
}
