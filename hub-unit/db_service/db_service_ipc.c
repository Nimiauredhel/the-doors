#include "db_service_ipc.h"
#include "db_service_db.h"

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
 * @brief Reads messages from the 'DB inbox' POSIX queue,
 * and forwards them for further processing, which typically means
 * translating them to a database entry.
 * A generous timeout duration is given to allow the calling thread
 * to terminate if necessary.
 **/
static void ipc_in_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    bytes_transmitted = mq_timedreceive(hub_handles_ptr->database_service_inbox_handle, msg_buff, sizeof(msg_buff), NULL, &mq_timeout);

    if (bytes_transmitted <= 0)
    {
        int err = errno;

        if (err == ETIMEDOUT || err == EAGAIN || err == EWOULDBLOCK)
        {
        }
        else
        {
            snprintf(log_buff, sizeof(log_buff), "Failed to receive from inbox: %s", strerror(err));
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
        db_append((DoorPacket_t *)msg_buff);
    }
}

void* ipc_in_task(void *arg)
{
    /// suppresses the 'unused variable' warning
    (void)arg;

    while(!should_terminate) ipc_in_loop();
    return NULL;
}

void ipc_init(void)
{
    log_append("Initializing IPC");

    bool initialized = ipc_init_inbox_handles()
                    && (NULL != (hub_handles_ptr = ipc_get_hub_handles_ptr()));

    if (initialized)
    {
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
    ipc_deinit_inbox_handles();

    if (hub_handles_ptr != NULL)
    {
        hub_handles_ptr = NULL;
    }
}
