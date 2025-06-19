#include "server.h"

static int server_socket = -1;
static struct sockaddr_in server_addr;

static uint8_t client_count = 0;
static int8_t next_slot_idx = 0;

static pthread_t listen_thread_handle;
static pthread_t ipc_thread_handle;

static pthread_mutex_t slots_mutex;
static ClientData_t client_slots[CLIENT_SLOTS];

static int clients_to_doors_shmid = -1; 
static HubShmLayout_t *clients_to_doors_ptr = NULL; 
static sem_t *clients_to_doors_sem = NULL; 
static int doors_to_clients_shmid = -1; 
static HubShmLayout_t *doors_to_clients_ptr = NULL; 
static sem_t *doors_to_clients_sem = NULL; 

static HubQueue_t *clients_to_doors_queue = NULL;

static void ipc_init(void)
{
    syslog_append("Initializing IPC");

    clients_to_doors_shmid = shmget(CLIENTS_TO_DOORS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (clients_to_doors_shmid < 0)
    {
        perror("Failed to get shm with key 324");
        syslog_append("Failed to get shm with key 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_shmid = shmget(DOORS_TO_CLIENTS_SHM_KEY, SHM_PACKET_TOTAL_SIZE, IPC_CREAT | 0666);

    if (doors_to_clients_shmid < 0)
    {
        perror("Failed to get shm with key 423");
        syslog_append("Failed to get shm with key 423");
        exit(EXIT_FAILURE);
    }

    clients_to_doors_sem = sem_open(CLIENTS_TO_DOORS_SHM_SEM, O_CREAT, 0600, 0);

    if (clients_to_doors_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 324");
        syslog_append("Failed to open sem for shm 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_sem = sem_open(DOORS_TO_CLIENTS_SHM_SEM, O_CREAT, 0600, 0);

    if (doors_to_clients_sem == SEM_FAILED)
    {
        perror("Failed to open sem for shm 423");
        syslog_append("Failed to open sem for shm 423");
        exit(EXIT_FAILURE);
    }

    clients_to_doors_ptr = (HubShmLayout_t *)shmat(clients_to_doors_shmid, NULL, 0);

    if (clients_to_doors_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 324");
        syslog_append("Failed to acquire pointer for shm 324");
        exit(EXIT_FAILURE);
    }

    doors_to_clients_ptr = (HubShmLayout_t *)shmat(doors_to_clients_shmid, NULL, 0);

    if (doors_to_clients_ptr == NULL)
    {
        perror("Failed to acquire pointer for shm 423");
        syslog_append("Failed to acquire pointer for shm 423");
        exit(EXIT_FAILURE);
    }

    clients_to_doors_queue = hub_queue_create(128);

    if (clients_to_doors_queue == NULL)
    {
        perror("Failed to create Clients to Doors Queue");
        syslog_append("Failed to create Clients to Doors Queue");
        exit(EXIT_FAILURE);
    }

    syslog_append("IPC Initialization Complete");
}

static void forward_door_to_client_request(DoorPacket_t *request)
{
    char log_buff[129] = {0};

    // temporarily hard-coded to forward to our one client
    // TODO: use destination index
    syslog_append("Forwarding to first active client...");
    pthread_mutex_lock(&slots_mutex);

    for(int i = 0; i < CLIENT_SLOTS; i++)
    {
         if (client_slots[i].slot_state == SLOTSTATE_ACTIVE)
         {
	      sprintf(log_buff, "Active client found for request: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
	      syslog_append(log_buff);
              // TODO: check return value
              hub_queue_enqueue(client_slots[i].outbox, request);
              pthread_mutex_unlock(&slots_mutex);
              return;
         }
    }

    pthread_mutex_unlock(&slots_mutex);
}

static void ipc_loop(void)
{
    char log_buff[128] = {0};
    DoorPacket_t packet_buff = {0};

    sem_wait(doors_to_clients_sem);

    if (doors_to_clients_ptr->state == SHMSTATE_DIRTY)
    {
	// TODO: add details (request, src, dest)
	sprintf(log_buff, "Forwarding request from door to client.");
	syslog_append(log_buff);
        memcpy(&packet_buff, &doors_to_clients_ptr->content, sizeof(DoorPacket_t));
        doors_to_clients_ptr->state = SHMSTATE_CLEAN;
        sem_post(doors_to_clients_sem);
        forward_door_to_client_request(&packet_buff);
    }
    else
    {
        sem_post(doors_to_clients_sem);
    }

    if (hub_queue_dequeue(clients_to_doors_queue, &packet_buff) >= 0)
    {
        bool sent = false;

        while(!sent)
        {
            sem_wait(clients_to_doors_sem);
            if (clients_to_doors_ptr->state == SHMSTATE_CLEAN)
            {
                memcpy(&clients_to_doors_ptr->content, &packet_buff, sizeof(DoorPacket_t));
                clients_to_doors_ptr->state = SHMSTATE_DIRTY;
                sent = true;
            }
            sem_post(clients_to_doors_sem);
        }
    }
}

static void init_server_socket(void)
{
    static const int reuse_flag = 1;

    char buff[64] = {0};
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket <= 0)
    {
        sprintf(buff, "Failed to create requests socket");
        syslog_append(buff);
        perror("Failed to create requests socket");
        exit(EXIT_FAILURE);
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,  &reuse_flag, sizeof(reuse_flag)))
    {
        sprintf(buff, "Failed to set socket 'reuse address' option");
        syslog_append(buff);
        perror("Failed to set socket 'reuse address' option");
        exit(EXIT_FAILURE);
    }

    if(0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT,  &reuse_flag, sizeof(reuse_flag)))
    {
        sprintf(buff, "Failed to set socket 'reuse port' option");
        syslog_append(buff);
        perror("Failed to set socket 'reuse port' option");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int ret = bind(server_socket, (struct sockaddr *)&(server_addr), sizeof(server_addr));

    if (ret != 0)
    {
        perror("Could not bind requests socket");
        sprintf(buff, "Could not bind requests socket");
        syslog_append(buff);
        exit(EXIT_FAILURE);
    }

    sprintf(buff, "Successfully bound socket.");
    syslog_append(buff);

    ret = listen(server_socket, 64);

    if (ret != 0)
    {
        perror("Failed to listen on socket.");
        sprintf(buff, "Failed to listen on socket.");
        syslog_append(buff);
        exit(EXIT_FAILURE);
    }

    sprintf(buff, "Now listening on socket.");
    syslog_append(buff);
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
    //sprintf(log_buff, "Checking outbox for client %s", inet_ntoa(data->client_addr.sin_addr));
    //syslog_append(log_buff);

    while(hub_queue_dequeue(data->outbox, &packet_buff) >= 0)
    {
	sprintf(log_buff, "Sending packet to client %s", inet_ntoa(data->client_addr.sin_addr));
	syslog_append(log_buff);

	ret = sendto(data->client_socket, &packet_buff, sizeof(DoorPacket_t), 0, (struct sockaddr *)&data->client_addr, data->client_addr_len);

        if (ret > 0)
        {
		perror("Sent Door Packet to client socket.");
		sprintf(log_buff, "Sent Door Packet to client socket.");
		syslog_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            sleep(1);
        }
        else
	{
		perror("Failed to send Door Packet to client socket.");
		sprintf(log_buff, "Failed to send Door Packet to client socket.");
		syslog_append(log_buff);
	}
    }
}

static void handle_report_packet(DoorPacket_t *packet)
{
}

static void forward_client_to_door_request(DoorPacket_t *request)
{
    bool sent = false;

    while(!sent)
    {
        sem_wait(clients_to_doors_sem);
        if (clients_to_doors_ptr->state == SHMSTATE_CLEAN)
        {
            memcpy(&clients_to_doors_ptr->content, request, sizeof(DoorPacket_t));
            clients_to_doors_ptr->state = SHMSTATE_DIRTY;
            sent = true;
        }
        sem_post(clients_to_doors_sem);
    }
}

static void connection_handle_incoming_packet(DoorPacket_t *packet)
{
    char log_buff[64] = {0};

    sprintf(log_buff, "Received packet with category #%d", packet->header.category);
    syslog_append(log_buff);

    switch(packet->header.category)
    {
    case PACKET_CAT_REPORT:
        syslog_append("Handling report packet");
        handle_report_packet(packet);
        break;
    case PACKET_CAT_REQUEST:
        syslog_append("Forwarding request to door");
        forward_client_to_door_request(packet);
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

	sprintf(log_buff, "Started task for client %s", inet_ntoa(data->client_addr.sin_addr));
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
		    sprintf(log_buff, "Received packet from client %s", inet_ntoa(data->client_addr.sin_addr));
		    syslog_append(log_buff);
		    connection_handle_incoming_packet(&packet_buff);
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
		}
		else
		{
		    error_counter++;
		    perror("Failed to receive on client socket.");
		    sprintf(log_buff, "Failed to receive on client socket.");
		    syslog_append(log_buff);

		    if (error_counter > error_threshold) break;
		}
	}

	sprintf(log_buff, "Releasing client %s", inet_ntoa(data->client_addr.sin_addr));
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
    pthread_mutex_lock(&slots_mutex);
    close(data->client_socket);
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
    char buff[64] = {0};
    struct sockaddr_in new_client_addr;
    socklen_t new_client_addr_len = sizeof(new_client_addr);
    int new_client_socket = -1;
    static const struct timeval socket_timeout = { .tv_sec = 1, .tv_usec = 0 };

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
        sprintf(buff, "Failed to accept request on socket.");
        syslog_append(buff);
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
			if (client_slots[i].client_addr.sin_addr.s_addr == new_client_addr.sin_addr.s_addr)
			{
				new = false;
				sprintf(buff, "Client reconnected: %s", inet_ntoa(client_slots[i].client_addr.sin_addr));
				syslog_append(buff);
				break;
			}
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

		sprintf(buff, "New client connected: %s", inet_ntoa(client_slots[next_slot_idx].client_addr.sin_addr));
		syslog_append(buff);
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

void* ipc_task(void *arg)
{
    for(;;)
    {
        ipc_loop();
    }
    return NULL;
}

void server_start(void)
{
    server_init();
    pthread_create(&listen_thread_handle, NULL, listen_task, NULL);
    pthread_create(&ipc_thread_handle, NULL, ipc_task, NULL);

    for(;;);
}
