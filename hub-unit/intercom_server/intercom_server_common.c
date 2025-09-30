#include "intercom_server_common.h"

#include "intercom_server_ipc.h"
#include "intercom_server_listener.h"

static pthread_t ipc_out_thread_handle;
static pthread_t ipc_in_thread_handle;

HubQueue_t *clients_to_doors_queue = NULL;

static void server_init(void)
{
    log_append("Initializing common resources.");

    clients_to_doors_queue = hub_queue_create(HUB_MAX_LOG_MSG_LENGTH);

    if (clients_to_doors_queue == NULL)
    {
        log_append("Failed to create Clients to Doors Queue.");
        should_terminate = true;
        return;
    }

    log_append("Done initializing common resources.");
}

void common_update_intercom_list_txt(HubIntercomStates_t *client_states_ptr)
{
    static const char *intercom_list_txt_path = "logs/intercom-list.txt";

    FILE *file = fopen(intercom_list_txt_path, "w");

    if (file == NULL) return;

    time_t t_now = time(NULL);
    struct tm tm_now = get_datetime();
    uint16_t counted = 0;

    fprintf(file, "Intercom Count: %u\n", client_states_ptr->count);
    fprintf(file, "Logged [%02u:%02u:%02u]\n\n", tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    for (uint16_t i = 0; i < HUB_MAX_INTERCOM_COUNT; i++)
    {
        if (counted >= client_states_ptr->count) break;

        if (client_states_ptr->last_seen[i] <= 0)
        {
            continue;
        }

        counted++;

        fprintf(file, "[%u] [%02X:%02X:%02X:%02X:%02X:%02X:] %s [Updated %lds ago]\n", i,
                client_states_ptr->mac_addresses[i][0], client_states_ptr->mac_addresses[i][1],
                client_states_ptr->mac_addresses[i][2], client_states_ptr->mac_addresses[i][3],
                client_states_ptr->mac_addresses[i][4], client_states_ptr->mac_addresses[i][5],
                client_states_ptr->name[i],     t_now - client_states_ptr->last_seen[i]);
    }

    fclose(file);
}

void server_start(void)
{
    /// TODO: replace frequent 'if()' with something nicer
    server_init();
    if (!should_terminate) ipc_init();
    if (!should_terminate) listener_init();

    if (!should_terminate) 
    {
        pthread_create(&ipc_out_thread_handle, NULL, ipc_out_task, NULL);
        pthread_create(&ipc_in_thread_handle, NULL, ipc_in_task, NULL);

        // not creating a new thread since main thread is doing nothing
        listener_task(NULL);
    }

    while(!should_terminate);
}
