#include "door_manager_ipc.h"

static mqd_t ipc_inbox_handle;
static mqd_t ipc_outbox_handle;

static void ipc_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    bytes_transmitted = mq_receive(ipc_inbox_handle, msg_buff, sizeof(msg_buff), NULL);

    if (bytes_transmitted < 0)
    {
        snprintf(log_buff, sizeof(log_buff), "Failed to receive from inbox: %s", strerror(errno));
        syslog_append(log_buff);
    }
    else
    {
        i2c_forward_request((DoorPacket_t *)msg_buff);
    }

    if (hub_queue_dequeue(doors_to_clients_queue, (DoorPacket_t *)&msg_buff) >= 0)
    {
        syslog_append("Forwarding from internal queue to outbox.");

        for(;;)
        {
            bytes_transmitted = mq_send(ipc_outbox_handle, msg_buff, sizeof(msg_buff), 0);

            if (bytes_transmitted >= 0) break;

            snprintf(log_buff, sizeof(log_buff), "Failed forwarding to outbox: %s", strerror(errno));
            syslog_append(log_buff);
            sleep(1);
        }
    }
}

void ipc_terminate(void)
{
    mq_close(ipc_inbox_handle);
    mq_close(ipc_outbox_handle);
}

void* ipc_task(void *arg)
{
    for(;;) ipc_loop();
    return NULL;
}

void ipc_init(void)
{
    syslog_append("Initializing IPC");

    ipc_outbox_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_WRONLY);

    if (ipc_outbox_handle < 0)
    {
        perror("Failed to open outbox queue");
        syslog_append("Failed to open outbox queue");
        exit(EXIT_FAILURE);
    }

    syslog_append("Opened outbox queue");

    ipc_inbox_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_RDONLY);

    if (ipc_outbox_handle < 0)
    {
        perror("Failed to open inbox queue");
        syslog_append("Failed to open inbox queue");
        exit(EXIT_FAILURE);
    }

    syslog_append("Opened inbox queue");

    syslog_append("IPC Initialization Complete");

}
