#include "server.h"

static int server_socket = -1;
static struct sockaddr_in server_addr;

static uint8_t client_count = 0;
static int8_t next_slot_idx = 0;

static pthread_mutex_t slots_mutex;
static ClientData_t client_slots[CLIENT_SLOTS];

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

void connection_loop(ClientData_t *data)
{
    static const uint8_t error_threshold = 30;

    static uint8_t error_counter = 0;

	int ret;
	char log_buff[128] = {0};
	char comm_buff[64] = {0};

	for(;;)
	{
        ret = 0;
        explicit_bzero(log_buff, sizeof(log_buff));
        explicit_bzero(comm_buff, sizeof(comm_buff));

		ret = recvfrom(data->client_socket, comm_buff, sizeof(comm_buff), 0, (struct sockaddr*)&data->client_addr, &data->client_addr_len);

        if (ret > 0)
        {
            error_counter = 0;
            sprintf(log_buff, "Received message: %s\n", comm_buff);
            syslog_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            sleep(1);
        }
        else
		{
            error_counter++;
			perror("Failed to receive on client socket.");
			sprintf(log_buff, "Failed to receive on client socket.");
			syslog_append(log_buff);

            if (error_counter > error_threshold) break;
		}

		sprintf(comm_buff, "Hello back, this is server.");
		ret = sendto(data->client_socket, comm_buff, strlen(log_buff)+1, 0, (struct sockaddr *)&data->client_addr, data->client_addr_len);

        if (ret > 0)
        {
            error_counter = 0;
			perror("Sent reply to client socket.");
			sprintf(log_buff, "Sent reply to client socket.");
			syslog_append(log_buff);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            sleep(1);
        }
        else
		{
            error_counter++;
			perror("Failed to send to sub socket.");
			sprintf(log_buff, "Failed to send to sub socket.");
			syslog_append(log_buff);

            if (error_counter > error_threshold) break;
		}
	}
}

void *connection_task(void *arg)
{
    ClientData_t *data = (ClientData_t *)arg; 

    pthread_mutex_lock(&slots_mutex);
    data->slot_state = SLOTSTATE_ACTIVE;
    pthread_mutex_unlock(&slots_mutex);

    connection_loop(data);

    // cleanup
    pthread_mutex_lock(&slots_mutex);
    close(data->client_socket);
    data->slot_state = SLOTSTATE_GARBAGE;
    pthread_mutex_unlock(&slots_mutex);

    return NULL;
}

static void server_init(void)
{
    init_server_socket();
    init_client_slots();
}

static void server_loop(void)
{
    char buff[64] = {0};
    struct sockaddr_in new_client_addr;
    socklen_t new_client_addr_len;
    int new_client_socket = -1;

    check_client_slots();

    if (next_slot_idx < 0 || client_count >= CLIENT_SLOTS)
    {
        sleep(2);
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
        pthread_mutex_lock(&slots_mutex);
        client_slots[next_slot_idx].slot_state = SLOTSTATE_TAKEN;
        client_slots[next_slot_idx].client_socket = new_client_socket;
        client_slots[next_slot_idx].client_addr = new_client_addr;
        client_slots[next_slot_idx].client_addr_len = new_client_addr_len;
        client_count++;
        pthread_create(&client_slots[next_slot_idx].client_thread_handle, NULL, connection_task, &client_slots[next_slot_idx]);
        pthread_mutex_unlock(&slots_mutex);
    }
}

void server_start(void)
{
    server_init();

    for(;;)
    {
        server_loop();
    }
}
