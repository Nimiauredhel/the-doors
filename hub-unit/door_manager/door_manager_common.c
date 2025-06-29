#include "door_manager_common.h"

static const char door_passes_path[16] = "./door_passes";
static const char door_list_path[16] = "./door_list";

static DoorInfo_t door_list[TARGET_ADDR_MAX_COUNT] = {0};

HubQueue_t *doors_to_clients_queue;
HubQueue_t *clients_to_doors_queue;

static void load_door_list(void)
{
    FILE *door_list_stream = fopen(door_list_path, "r");

    if (door_list_stream == NULL)
    {
        door_list_stream = fopen(door_list_path, "w+");
    }

    // TODO: actually read file to data structure

    fclose(door_list_stream);
}

static void load_door_passes(void)
{
    FILE *door_passes_stream = fopen(door_passes_path, "r");

    if (door_passes_stream == NULL)
    {
        door_passes_stream = fopen(door_passes_path, "w+");
    }

    // TODO: actually read file to data structure

    fclose(door_passes_stream);
}

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

    clients_to_doors_queue = hub_queue_create(128);

    if (clients_to_doors_queue == NULL)
    {
        perror("Failed to create Clients to Doors Queue");
        syslog_append("Failed to create Clients to Doors Queue");
        common_terminate(EXIT_FAILURE);
    }

    syslog_append("Done initializing common resources.");
}

void common_terminate(int ret)
{
    ipc_terminate();
    i2c_terminate();

    if (doors_to_clients_queue != NULL)
    {
        hub_queue_destroy(doors_to_clients_queue);
    }

    if (clients_to_doors_queue != NULL)
    {
        hub_queue_destroy(clients_to_doors_queue);
    }

    exit(ret);
}
