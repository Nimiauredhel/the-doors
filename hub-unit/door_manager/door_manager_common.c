#include "door_manager_common.h"

HubQueue_t *doors_to_clients_queue;

void common_init(void)
{
    syslog_append("Initializing common resources.");

    doors_to_clients_queue = hub_queue_create(128);

    if (doors_to_clients_queue == NULL)
    {
        perror("Failed to create Doors to Clients Queue");
        syslog_append("Failed to create Doors to Clients Queue");
        common_terminate(EXIT_FAILURE);
    }

    syslog_append("Done initializing common resources.");
}

void common_terminate(int ret)
{
    ipc_deinit();
    i2c_terminate();

    if (doors_to_clients_queue != NULL)
    {
        hub_queue_destroy(doors_to_clients_queue);
    }

    exit(ret);
}
