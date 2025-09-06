#include "door_manager_common.h"

HubQueue_t *doors_to_clients_queue;

void common_update_door_list_txt(HubDoorStates_t *door_states_ptr)
{
    static const char *door_list_txt_path = "logs/door-list.txt";

    time_t t_now = time(NULL);
    struct tm tm_now = get_datetime();
    FILE *file = fopen(door_list_txt_path, "w");

    if (file == NULL) return;

    fprintf(file, "Door Count: %u\n", door_states_ptr->count);
    fprintf(file, "Logged [%02u:%02u:%02u]\n\n", tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    for (int i = 0; i < door_states_ptr->count; i++)
    {
        fprintf(file, " [%u] %s [Updated %lds ago]\n", door_states_ptr->id[i], door_states_ptr->name[i], t_now - door_states_ptr->last_seen[i]);
    }

    fclose(file);
}

void common_init(void)
{
    log_append("Initializing common resources.");

    doors_to_clients_queue = hub_queue_create(128);

    if (doors_to_clients_queue == NULL)
    {
        perror("Failed to create Doors to Clients Queue");
        log_append("Failed to create Doors to Clients Queue");
        common_terminate(EXIT_FAILURE);
    }

    log_append("Done initializing common resources.");
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
