#include "intercom_server_common.h"
#include "intercom_server_listener.h"
#include "intercom_server_ipc.h"

static const struct timespec listener_loop_delay =
{
    .tv_nsec = 500000000,
    .tv_sec = 0,
};

static int server_socket = -1;
static struct sockaddr_in server_addr;

static uint8_t client_count = 0;
static int8_t next_slot_idx = 0;

static pthread_mutex_t slots_mutex;
static ClientData_t client_slots[HUB_MAX_CLIENT_COUNT];

void send_request(DoorRequest_t request, ClientData_t *client)
{
	struct tm datetime = get_datetime();

	DoorPacket_t packet =
	{
	    .header.time = packet_encode_time(datetime.tm_hour, datetime.tm_min, datetime.tm_sec),
	    .header.date = packet_encode_date(datetime.tm_year, datetime.tm_mon, datetime.tm_mday),
	    .header.category = PACKET_CAT_REQUEST,
	    .body.Request.request_id = request,
	};

	hub_queue_enqueue(client->outbox, &packet);
}

void forward_door_to_client_request(DoorPacket_t *request)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    // temporarily hard-coded to forward to our one client
    // TODO: use destination index
    log_append("Forwarding to first active client...");
    pthread_mutex_lock(&slots_mutex);

    for(int i = 0; i < HUB_MAX_CLIENT_COUNT; i++)
    {
         if (client_slots[i].slot_state == SLOTSTATE_ACTIVE)
         {
              snprintf(log_buff, sizeof(log_buff), "Active client found for request: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
              log_append(log_buff);

              // TODO: check return value
              hub_queue_enqueue(client_slots[i].outbox, request);
              pthread_mutex_unlock(&slots_mutex);
              return;
         }
    }

    pthread_mutex_unlock(&slots_mutex);
}

static void init_server_socket(void)
{
    static const int reuse_flag = 1;

    /// TODO: fix up error handling and graceful termination flow - the current state is half baked at best

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("Failed to create requests socket");

        snprintf(log_buff, sizeof(log_buff), "Failed to create requests socket");
        log_append(log_buff);

        should_terminate = true;
        return;
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse address' option");

        snprintf(log_buff, sizeof(log_buff), "Failed to set socket 'reuse address' option");
        log_append(log_buff);

        should_terminate = true;
        return;
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse port' option");

        snprintf(log_buff, sizeof(log_buff), "Failed to set socket 'reuse port' option");
        log_append(log_buff);

        should_terminate = true;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr *)&(server_addr), sizeof(server_addr));

    if (ret != 0)
    {
        perror("Could not bind requests socket");

        snprintf(log_buff, sizeof(log_buff), "Could not bind requests socket");
        log_append(log_buff);

        should_terminate = true;
        return;
    }

    snprintf(log_buff, sizeof(log_buff), "Successfully bound socket.");
    log_append(log_buff);

    ret = listen(server_socket, 64);

    if (ret != 0)
    {
        perror("Failed to listen on socket.");

        snprintf(log_buff, sizeof(log_buff), "Failed to listen on socket.");
        log_append(log_buff);

        should_terminate = true;
        return;
    }

    snprintf(log_buff, sizeof(log_buff), "Now listening on socket.");
    log_append(log_buff);
}

static void init_client_slots(void)
{
    pthread_mutex_init(&slots_mutex, NULL);

    for (int i = 0; i < HUB_MAX_CLIENT_COUNT; i++)
    {
        client_slots[i].slot_state = SLOTSTATE_VACANT;
        client_slots[i].client_socket = -1;
        client_slots[i].index = -1;
    }
}

static void increment_next_client_slot(void)
{
    if (client_count >= HUB_MAX_CLIENT_COUNT)
    {
        next_slot_idx = -1;
        return;
    }

    next_slot_idx++;

    if (next_slot_idx > HUB_MAX_CLIENT_COUNT) next_slot_idx = 0;

    pthread_mutex_lock(&slots_mutex);

    if (client_slots[next_slot_idx].slot_state == SLOTSTATE_VACANT)
    {
        pthread_mutex_unlock(&slots_mutex);
        return;
    }

    do
    {
        next_slot_idx++;
    } while(next_slot_idx < HUB_MAX_CLIENT_COUNT && client_slots[next_slot_idx].slot_state != SLOTSTATE_VACANT);

    if (next_slot_idx > HUB_MAX_CLIENT_COUNT) next_slot_idx = -1;

    pthread_mutex_unlock(&slots_mutex);
}

static void update_client_slots(bool acquire_slots_mutex)
{
    if (acquire_slots_mutex) pthread_mutex_lock(&slots_mutex);

    for (int i = 0; i < HUB_MAX_CLIENT_COUNT; i++)
    {
        if (client_slots[i].slot_state == SLOTSTATE_GARBAGE)
        {
            pthread_join(client_slots[i].client_thread_handle, NULL);
            client_slots[i].slot_state = SLOTSTATE_VACANT;
            client_slots[i].client_socket = -1;
            client_slots[i].index = -1;
        }
    }

    if (acquire_slots_mutex) pthread_mutex_unlock(&slots_mutex);

    if (next_slot_idx < 0) increment_next_client_slot();
}

static void connection_check_outbox(ClientData_t *data)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    DoorPacket_t packet_buff = {0};
    int ret;

    while(hub_queue_dequeue(data->outbox, &packet_buff) >= 0)
    {
        snprintf(log_buff, sizeof(log_buff), "Sending packet to client %s", inet_ntoa(data->client_addr.sin_addr));
        log_append(log_buff);

        ret = sendto(data->client_socket, &packet_buff, sizeof(DoorPacket_t), 0, (struct sockaddr *)&data->client_addr, data->client_addr_len);

        if (ret > 0)
        {
            snprintf(log_buff, sizeof(log_buff), "Sent Door Packet to client socket.");
            log_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            nanosleep(&listener_loop_delay, NULL);
        }
        else
        {
            perror(log_buff);

            snprintf(log_buff, sizeof(log_buff), "Failed to send Door Packet to client socket.");
            log_append(log_buff);
        }
    }
}

static void connection_send_door_list(ClientData_t *data)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    uint8_t out_buff[sizeof(DoorPacket_t) + sizeof(DoorInfo_t)] = {0};
    DoorPacket_t *packet_ptr = (DoorPacket_t *)out_buff;
    DoorInfo_t *data_ptr = (DoorInfo_t *)(out_buff + sizeof(DoorPacket_t));
    int ret;

    snprintf(log_buff, sizeof(log_buff), "Sending door list to client.");
    log_append(log_buff);

    HubDoorStates_t *door_list = ipc_acquire_door_states_ptr();

	struct tm datetime = get_datetime();

    packet_ptr->header.time = packet_encode_time(datetime.tm_hour, datetime.tm_min, datetime.tm_sec);
    packet_ptr->header.date = packet_encode_date(datetime.tm_year, datetime.tm_mon, datetime.tm_mday);
    packet_ptr->header.category = PACKET_CAT_DATA;
    packet_ptr->body.Data.data_type = PACKET_DATA_DOOR_INFO;

    for (int i = 0; i < door_list->count; i++)
    {
        data_ptr->index = i;
        strncpy(data_ptr->name, door_list->name[i], UNIT_NAME_MAX_LEN);

        ret = sendto(data->client_socket, out_buff, sizeof(out_buff), 0, (struct sockaddr *)&data->client_addr, data->client_addr_len);

        if (ret > 0)
        {
            snprintf(log_buff, sizeof(log_buff), "Sent Door Info to client socket: index %u, name %s.", i, door_list->name[i]);
            log_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            nanosleep(&listener_loop_delay, NULL);
        }
        else
        {
            perror(log_buff);

            snprintf(log_buff, sizeof(log_buff), "Failed to send Door Info to client socket.");
            log_append(log_buff);
        }
    }

    ipc_release_door_states_ptr();

    snprintf(log_buff, sizeof(log_buff), "Sent door list to client.");
    log_append(log_buff);
}

static void process_request_from_intercom(DoorPacket_t *packet, ClientData_t *client)
{
    log_append("Handling request packet.");

	switch(packet->body.Request.request_id)
	{
	    case(PACKET_REQUEST_SYNC_TIME):
            log_append("Responding to time sync request.");
            send_request(PACKET_REQUEST_SYNC_TIME, client);
            connection_send_door_list(client);
            break;
	    default:
            log_append("Forwarding request to door.");
            hub_queue_enqueue(clients_to_doors_queue, packet);
            break;
	}
}

static void connection_update_client_info(ClientInfo_t *info, ClientData_t *client)
{
    HubClientStates_t *intercom_states_ptr = ipc_acquire_intercom_states_ptr();

    intercom_states_ptr->last_seen[client->index] = time(NULL);
    intercom_states_ptr->slot_used[client->index] = true;

    for (uint8_t i = 0; i < 6; i++)
    {
        intercom_states_ptr->mac_addresses[client->index][i] = info->mac_address[i];
    }

    strncpy(intercom_states_ptr->name[client->index], info->name, sizeof(intercom_states_ptr->name));

    common_update_intercom_list_txt(intercom_states_ptr);

    ipc_release_intercom_states_ptr();
}

static void connection_remove_client_info(ClientData_t *client)
{
    HubClientStates_t *intercom_states_ptr = ipc_acquire_intercom_states_ptr();

    intercom_states_ptr->slot_used[client->index] = false;

    common_update_intercom_list_txt(intercom_states_ptr);

    ipc_release_intercom_states_ptr();
}

static void connection_handle_incoming_packet(DoorPacket_t *packet_ptr, ClientInfo_t *info_ptr, ClientData_t *client)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    snprintf(log_buff, sizeof(log_buff), "Received packet with category #%d", packet_ptr->header.category);
    log_append(log_buff);

    switch(packet_ptr->header.category)
    {
    case PACKET_CAT_REPORT:
        log_append("Received report packet (cannot handle, unimplemented).");
        /// TODO: decide whether intercom units should send report packets
        break;
    case PACKET_CAT_REQUEST:
        process_request_from_intercom(packet_ptr, client);
        break;
    case PACKET_CAT_DATA:
        connection_update_client_info(info_ptr, client);
        break;
    case PACKET_CAT_NONE:
    case PACKET_CAT_MAX:
    default:
        log_append("Packet category invalid");
      break;
    }
}

static void connection_loop(ClientData_t *data)
{
	const uint8_t error_threshold = 100;
	const time_t silence_threshold = 15;

	uint8_t error_counter = 0;

	char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
	int ret;
    time_t last_seen = time(NULL);

    uint8_t rx_buff[sizeof(DoorPacket_t) + DOOR_DATA_BYTES_SMALL];
    DoorPacket_t *rx_packet_ptr = (DoorPacket_t *)rx_buff;
    ClientInfo_t *rx_info_ptr = (ClientInfo_t *)(rx_buff + sizeof(DoorPacket_t));

	snprintf(log_buff, sizeof(log_buff), "Started task for client %s", inet_ntoa(data->client_addr.sin_addr));
	log_append(log_buff);

    connection_send_door_list(data);
    last_seen = time(NULL);

    while(!should_terminate)
	{
		connection_check_outbox(data);

		ret = 0;
		explicit_bzero(log_buff, sizeof(log_buff));
		explicit_bzero(rx_buff, sizeof(rx_buff));

		ret = recvfrom(data->client_socket, rx_buff, sizeof(rx_buff), 0, (struct sockaddr*)&data->client_addr, &data->client_addr_len);

		if (ret > 0)
		{
            last_seen = time(NULL);
		    error_counter = 0;
		    snprintf(log_buff, sizeof(log_buff), "Received packet from client %s", inet_ntoa(data->client_addr.sin_addr));
		    log_append(log_buff);
		    connection_handle_incoming_packet(rx_packet_ptr, rx_info_ptr, data);
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
            nanosleep(&listener_loop_delay, NULL);
		}
		else
		{
            int err = errno;
		    error_counter++;

		    snprintf(log_buff, sizeof(log_buff), "Failed to receive on client socket: %s.", strerror(err));
		    log_append(log_buff);

		}

        if (((time(NULL) - last_seen) > silence_threshold) || (error_counter > error_threshold)) break;
	}

	snprintf(log_buff, sizeof(log_buff), "Releasing client %s", inet_ntoa(data->client_addr.sin_addr));
	log_append(log_buff);
}

void *connection_task(void *arg)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    ClientData_t *data = (ClientData_t *)arg; 

    snprintf(log_buff, sizeof(log_buff), "Starting connection task, client count: %d.", client_count);
    log_append(log_buff);

    pthread_mutex_lock(&slots_mutex);
    // TODO: null check
    data->outbox = hub_queue_create(8);
    data->slot_state = SLOTSTATE_ACTIVE;
    // TODO: handle slots updates in a separate thread and only mark "dirty" from here
    update_client_slots(false);
    pthread_mutex_unlock(&slots_mutex);

    snprintf(log_buff, sizeof(log_buff), "Starting connection loop, client count: %d.", client_count);
    log_append(log_buff);

    connection_loop(data);

    snprintf(log_buff, sizeof(log_buff), "Ended connection loop, client count: %d.", client_count);
    log_append(log_buff);

    // cleanup
    close(data->client_socket);

    connection_remove_client_info(data);

    pthread_mutex_lock(&slots_mutex);
    hub_queue_destroy(data->outbox);
    data->slot_state = SLOTSTATE_GARBAGE;
    // TODO: handle slots updates in a separate thread and only mark "dirty" from here
    update_client_slots(false);
    pthread_mutex_unlock(&slots_mutex);

    snprintf(log_buff, sizeof(log_buff), "Ended connection task, client count: %d.", client_count);
    log_append(log_buff);

    return NULL;
}

static void listen_loop(void)
{
    static const struct timeval socket_timeout = { .tv_sec = 1, .tv_usec = 0 };

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    struct sockaddr_in new_client_addr;
    socklen_t new_client_addr_len = sizeof(new_client_addr);
    int new_client_socket = -1;

    if (next_slot_idx < 0 || client_count >= HUB_MAX_CLIENT_COUNT)
    {
        // TODO: put this check after accept() and handle with some sort of response
        nanosleep(&listener_loop_delay, NULL);
        return;
    }

    new_client_socket = accept(server_socket, (struct sockaddr *)&new_client_addr, &new_client_addr_len);

    if (new_client_socket < 0)
    {
        int err = errno;

        snprintf(log_buff, sizeof(log_buff), "Failed to accept request on socket: %s.", strerror(err));
        log_append(log_buff);

        nanosleep(&listener_loop_delay, NULL);
    }
    else
    {
        bool new = true;
        pthread_mutex_lock(&slots_mutex);

        for (int i = 0; i < HUB_MAX_CLIENT_COUNT; i++)
        {
            if (client_slots[i].slot_state == SLOTSTATE_ACTIVE || client_slots[i].slot_state == SLOTSTATE_TAKEN)
            {
                /*
                if (client_slots[i].client_addr.sin_addr.s_addr == new_client_addr.sin_addr.s_addr)
                {
                    new = false;

                    snprintf(log_buff, sizeof(log_buff), "Client reconnected: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
                    log_append(log_buff);

                    send_request(PACKET_REQUEST_SYNC_TIME, &client_slots[i]);
                    break;
                }
                */
            }
        }

        if (new)
        {
            client_slots[next_slot_idx].slot_state = SLOTSTATE_TAKEN;
            client_slots[next_slot_idx].client_socket = new_client_socket;
            client_slots[next_slot_idx].client_addr = new_client_addr;
            client_slots[next_slot_idx].client_addr_len = new_client_addr_len;
            client_slots[next_slot_idx].index = next_slot_idx;
            client_count++;
            pthread_create(&client_slots[next_slot_idx].client_thread_handle, NULL, connection_task, &client_slots[next_slot_idx]);
            pthread_mutex_unlock(&slots_mutex);

            if(0 > setsockopt(new_client_socket, SOL_SOCKET, SO_RCVTIMEO,  &socket_timeout, sizeof(socket_timeout)))
            {
                int err = errno;
                snprintf(log_buff, sizeof(log_buff), "Failed to set socket timeout: %s.", strerror(err));
                log_append(log_buff);
                // TODO: actually handle this
            }

            snprintf(log_buff, sizeof(log_buff), "New client connected: %s", inet_ntoa(client_slots[next_slot_idx].client_addr.sin_addr));
            log_append(log_buff);

            increment_next_client_slot();
        }
    }
}

void listener_init(void)
{
    init_server_socket();
    if (!should_terminate) init_client_slots();
}

void* listener_task(void *arg)
{
    // suppresses 'unused variable' warning
    (void)arg;

    log_append("Starting Listener Task.");

    while(!should_terminate)
    {
        listen_loop();
    }

    log_append("Ending Listener Task.");

    return NULL;
}
