#include "server.h"

static int server_socket = -1;
static struct sockaddr_in server_addr;

static uint8_t client_count = 0;
static int8_t next_slot_idx = 0;

static pthread_t listen_thread_handle;
static pthread_t ipc_out_thread_handle;

static pthread_mutex_t slots_mutex;
static ClientData_t client_slots[CLIENT_SLOTS];

static mqd_t ipc_inbox_handle;
static mqd_t ipc_outbox_handle;

static HubQueue_t *clients_to_doors_queue = NULL;

static void ipc_init(void)
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

static void send_request(DoorRequest_t request, ClientData_t *client)
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

static void forward_door_to_client_request(DoorPacket_t *request)
{
    char log_buff[128] = {0};

    // temporarily hard-coded to forward to our one client
    // TODO: use destination index
    syslog_append("Forwarding to first active client...");
    pthread_mutex_lock(&slots_mutex);

    for(int i = 0; i < CLIENT_SLOTS; i++)
    {
         if (client_slots[i].slot_state == SLOTSTATE_ACTIVE)
         {
              snprintf(log_buff, sizeof(log_buff), "Active client found for request: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
              syslog_append(log_buff);

              // TODO: check return value
              hub_queue_enqueue(client_slots[i].outbox, request);
              pthread_mutex_unlock(&slots_mutex);
              return;
         }
    }

    pthread_mutex_unlock(&slots_mutex);
}

static void ipc_in_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    bytes_transmitted = mq_receive(ipc_inbox_handle, msg_buff, sizeof(msg_buff), NULL);

    if (bytes_transmitted <= 0)
    {
        snprintf(log_buff, sizeof(log_buff), "Failed to receive from inbox: %s", strerror(errno));
        syslog_append(log_buff);
        sleep(1);
    }
    else
    {
        forward_door_to_client_request((DoorPacket_t *)&msg_buff);
    }
}

static void ipc_out_loop(void)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    if (hub_queue_dequeue(clients_to_doors_queue, (DoorPacket_t *)&msg_buff) >= 0)
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

static void init_server_socket(void)
{
    static const int reuse_flag = 1;

    char log_buff[128] = {0};
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket <= 0)
    {
        perror("Failed to create requests socket");

        snprintf(log_buff, sizeof(log_buff), "Failed to create requests socket");
        syslog_append(log_buff);

        exit(EXIT_FAILURE);
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse address' option");

        snprintf(log_buff, sizeof(log_buff), "Failed to set socket 'reuse address' option");
        syslog_append(log_buff);

        exit(EXIT_FAILURE);
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse port' option");

        snprintf(log_buff, sizeof(log_buff), "Failed to set socket 'reuse port' option");
        syslog_append(log_buff);

        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr *)&(server_addr), sizeof(server_addr));

    if (ret != 0)
    {
        perror("Could not bind requests socket");

        snprintf(log_buff, sizeof(log_buff), "Could not bind requests socket");
        syslog_append(log_buff);

        exit(EXIT_FAILURE);
    }

    snprintf(log_buff, sizeof(log_buff), "Successfully bound socket.");
    syslog_append(log_buff);

    ret = listen(server_socket, 64);

    if (ret != 0)
    {
        perror("Failed to listen on socket.");

        snprintf(log_buff, sizeof(log_buff), "Failed to listen on socket.");
        syslog_append(log_buff);

        exit(EXIT_FAILURE);
    }

    snprintf(log_buff, sizeof(log_buff), "Now listening on socket.");
    syslog_append(log_buff);
}

static void init_client_slots(void)
{
    pthread_mutex_init(&slots_mutex, NULL);

    for (int i = 0; i < CLIENT_SLOTS; i++)
    {
        client_slots[i].slot_state = SLOTSTATE_VACANT;
        client_slots[i].client_socket = -1;
        //client_slots[i].next_slot_idx = -1;
    }
}

static void increment_next_client_slot(void)
{
    if (client_count >= CLIENT_SLOTS)
    {
        next_slot_idx = -1;
        return;
    }

    next_slot_idx++;

    if (next_slot_idx > CLIENT_SLOTS) next_slot_idx = 0;

    pthread_mutex_lock(&slots_mutex);

    if (client_slots[next_slot_idx].slot_state == SLOTSTATE_VACANT)
    {
        pthread_mutex_unlock(&slots_mutex);
        return;
    }

    do
    {
        next_slot_idx++;
    } while(next_slot_idx < CLIENT_SLOTS && client_slots[next_slot_idx].slot_state != SLOTSTATE_VACANT);

    if (next_slot_idx > CLIENT_SLOTS) next_slot_idx = -1;

    pthread_mutex_unlock(&slots_mutex);
}

static void check_client_slots(void)
{
    pthread_mutex_lock(&slots_mutex);

    for (int i = 0; i < CLIENT_SLOTS; i++)
    {
        if (client_slots[i].slot_state == SLOTSTATE_GARBAGE)
        {
            pthread_join(client_slots[i].client_thread_handle, NULL);
            client_slots[i].slot_state = SLOTSTATE_VACANT;
            client_slots[i].client_socket = -1;
            //client_slots[i].next_slot_idx = -1;
            client_count--;
        }
    }

    pthread_mutex_unlock(&slots_mutex);

    if (next_slot_idx < 0) increment_next_client_slot();
}

static void connection_check_outbox(ClientData_t *data)
{
    char log_buff[128] = {0};
    DoorPacket_t packet_buff = {0};
    uint16_t remaining;
    int ret;

    //snprintf(log_buff, sizeof(log_buff), "Checking outbox for client %s", inet_ntoa(data->client_addr.sin_addr));
    //syslog_append(log_buff);

    while(hub_queue_dequeue(data->outbox, &packet_buff) >= 0)
    {
        snprintf(log_buff, sizeof(log_buff), "Sending packet to client %s", inet_ntoa(data->client_addr.sin_addr));
        syslog_append(log_buff);

        ret = sendto(data->client_socket, &packet_buff, sizeof(DoorPacket_t), 0, (struct sockaddr *)&data->client_addr, data->client_addr_len);

        if (ret > 0)
        {
            snprintf(log_buff, sizeof(log_buff), "Sent Door Packet to client socket.");
            syslog_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // TODO: check possible consequences of sleep() after a timeout
            sleep(1);
        }
        else
        {
            perror(log_buff);

            snprintf(log_buff, sizeof(log_buff), "Failed to send Door Packet to client socket.");
            syslog_append(log_buff);
        }
    }
}

static void forward_client_to_door_request(DoorPacket_t *request)
{
    static char msg_buff[MQ_MSG_SIZE_MAX] = {0};
    static ssize_t bytes_transmitted = 0;
    static char log_buff[128] = {0};

    syslog_append("Forwarding from internal queue to outbox.");

    memcpy(msg_buff, request, sizeof(*request));

    for(;;)
    {
        bytes_transmitted = mq_send(ipc_outbox_handle, msg_buff, sizeof(msg_buff), 0);

        if (bytes_transmitted >= 0) break;

        snprintf(log_buff, sizeof(log_buff), "Failed forwarding to outbox: %s", strerror(errno));
        syslog_append(log_buff);
        sleep(1);
    }
}

static void handle_report_packet(DoorPacket_t *packet, ClientData_t *client)
{
    syslog_append("Handling report packet (unimplemented)");
}

static void handle_request_packet(DoorPacket_t *packet, ClientData_t *client)
{
    syslog_append("Handling request packet.");

	switch(packet->body.Request.request_id)
	{
	    case(PACKET_REQUEST_SYNC_TIME):
            syslog_append("Responding to time sync request.");
            send_request(PACKET_REQUEST_SYNC_TIME, client);
            break;
	    default:
            syslog_append("Forwarding request to door.");
            forward_client_to_door_request(packet);
            break;
	}
}

static void connection_handle_incoming_packet(DoorPacket_t *packet, ClientData_t *client)
{
    char log_buff[182] = {0};

    snprintf(log_buff, sizeof(log_buff), "Received packet with category #%d", packet->header.category);
    syslog_append(log_buff);

    switch(packet->header.category)
    {
    case PACKET_CAT_REPORT:
        handle_report_packet(packet, client);
        break;
    case PACKET_CAT_REQUEST:
        handle_request_packet(packet, client);
        break;
    case PACKET_CAT_NONE:
    case PACKET_CAT_DATA:
    case PACKET_CAT_MAX:
    default:
        syslog_append("Packet category invalid");
      break;
    }
}

static void connection_loop(ClientData_t *data)
{
	static const uint8_t error_threshold = 6;
	static uint8_t error_counter = 0;

	int ret;
	char log_buff[128] = {0};
	DoorPacket_t packet_buff = {0};

	snprintf(log_buff, sizeof(log_buff), "Started task for client %s", inet_ntoa(data->client_addr.sin_addr));
	syslog_append(log_buff);

	for(;;)
	{
		connection_check_outbox(data);

		ret = 0;
		explicit_bzero(log_buff, sizeof(log_buff));
		explicit_bzero(&packet_buff, sizeof(packet_buff));

		ret = recvfrom(data->client_socket, &packet_buff, sizeof(packet_buff), 0, (struct sockaddr*)&data->client_addr, &data->client_addr_len);

		if (ret > 0)
		{
		    error_counter = 0;
		    snprintf(log_buff, sizeof(log_buff), "Received packet from client %s", inet_ntoa(data->client_addr.sin_addr));
		    syslog_append(log_buff);
		    connection_handle_incoming_packet(&packet_buff, data);
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
            // TODO: check possible consequences of sleep() after a timeout
            sleep(1);
		}
		else
		{
		    perror("Failed to receive on client socket.");

		    error_counter++;

		    snprintf(log_buff, sizeof(log_buff), "Failed to receive on client socket.");
		    syslog_append(log_buff);


		    if (error_counter > error_threshold) break;
		}
	}

	snprintf(log_buff, sizeof(log_buff), "Releasing client %s", inet_ntoa(data->client_addr.sin_addr));
	syslog_append(log_buff);
}

void *connection_task(void *arg)
{
    ClientData_t *data = (ClientData_t *)arg; 

    pthread_mutex_lock(&slots_mutex);
    // TODO: null check
    data->outbox = hub_queue_create(8);
    data->slot_state = SLOTSTATE_ACTIVE;
    pthread_mutex_unlock(&slots_mutex);

    connection_loop(data);

    // cleanup
    close(data->client_socket);
    pthread_mutex_lock(&slots_mutex);
    hub_queue_destroy(data->outbox);
    data->slot_state = SLOTSTATE_GARBAGE;
    client_count--;
    pthread_mutex_unlock(&slots_mutex);

    return NULL;
}

static void server_init(void)
{
    ipc_init();
    init_server_socket();
    init_client_slots();
}

static void server_loop(void)
{
    static const struct timeval socket_timeout = { .tv_sec = 1, .tv_usec = 0 };

    char log_buff[128] = {0};

    struct sockaddr_in new_client_addr;
    socklen_t new_client_addr_len = sizeof(new_client_addr);
    int new_client_socket = -1;

    check_client_slots();

    if (next_slot_idx < 0 || client_count >= CLIENT_SLOTS)
    {
        sleep(1);
        return;
    }

    new_client_socket = accept(server_socket, (struct sockaddr *)&new_client_addr, &new_client_addr_len);

    if (new_client_socket < 0)
    {
        perror("Failed to accept request on socket.");

        snprintf(log_buff, sizeof(log_buff), "Failed to accept request on socket.");
        syslog_append(log_buff);

        sleep(1);
    }
    else
    {
        bool new = true;
        pthread_mutex_lock(&slots_mutex);

        for (int i = 0; i < CLIENT_SLOTS; i++)
        {
            if (client_slots[i].slot_state == SLOTSTATE_ACTIVE || client_slots[i].slot_state == SLOTSTATE_TAKEN)
            {
                /*
                if (client_slots[i].client_addr.sin_addr.s_addr == new_client_addr.sin_addr.s_addr)
                {
                    new = false;

                    snprintf(log_buff, sizeof(log_buff), "Client reconnected: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
                    syslog_append(log_buff);

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
            client_count++;
            pthread_create(&client_slots[next_slot_idx].client_thread_handle, NULL, connection_task, &client_slots[next_slot_idx]);
            pthread_mutex_unlock(&slots_mutex);

            if(0 > setsockopt(new_client_socket, SOL_SOCKET, SO_RCVTIMEO,  &socket_timeout, sizeof(socket_timeout)))
            {
                perror("Failed to set socket timeout");
                // TODO: figure out how to actually handle this
            }

            snprintf(log_buff, sizeof(log_buff), "New client connected: %s", inet_ntoa(client_slots[next_slot_idx].client_addr.sin_addr));
            syslog_append(log_buff);

            increment_next_client_slot();
        }
    }
}

void* listen_task(void *arg)
{
    for(;;)
    {
        server_loop();
    }
    return NULL;
}

void* ipc_in_task(void *arg)
{
    for(;;)
    {
        ipc_in_loop();
    }
    return NULL;
}

void* ipc_out_task(void *arg)
{
    for(;;)
    {
        ipc_out_loop();
    }
    return NULL;
}

void server_start(void)
{
    server_init();

    pthread_create(&listen_thread_handle, NULL, listen_task, NULL);
    pthread_create(&ipc_out_thread_handle, NULL, ipc_out_task, NULL);

    // not creating a new thread since main thread is doing nothing
    ipc_in_task(NULL);

    for(;;);
}
