#include "web_server_ipc.h"

static const struct timespec mq_timeout =
{
    .tv_nsec = 0,
    .tv_sec = 1,
};

static const HubHandles_t *hub_handles_ptr = NULL;

void ipc_init(void)
{
    log_append("Initializing IPC");

    bool initialized = ipc_init_inbox_handles()
                    && ipc_init_door_states_ptrs(false)
                    && ipc_init_intercom_states_ptrs(false)
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
        sem_close(hub_handles_ptr->door_states_sem_ptr);
        sem_close(hub_handles_ptr->intercom_states_sem_ptr);
        hub_handles_ptr = NULL;
    }
}
