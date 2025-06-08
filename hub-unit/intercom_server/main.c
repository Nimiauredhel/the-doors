#include "hub_common.h"
#include "networking_common.h"

int server_socket = -1;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
socklen_t addr_len;

static void server_init(void)
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

    client_addr.sin_family = AF_INET;

    int bind_result = bind(server_socket, (struct sockaddr *)&(server_addr), sizeof(server_addr));

    if (bind_result != 0)
    {
        perror("Could not bind requests socket");
        sprintf(buff, "Could not bind requests socket");
        syslog_append(buff);
        exit(EXIT_FAILURE);
    }

    sprintf(buff, "Successfully bound socket.");
    syslog_append(buff);
}

void connection_loop(int data_socket, struct sockaddr* client_addr, socklen_t *addr_len)
{
	int ret;
	char buff[64] = {0};
	char comm_buff[64] = {0};

	for(;;)
	{
		ret = recvfrom(data_socket, comm_buff, sizeof(comm_buff), 0, client_addr, addr_len);

		if (ret <=0)
		{
			perror("Failed to receive on sub socket.");
			sprintf(buff, "Failed to receive on sub socket.");
			syslog_append(buff);
			break;
		}

		sprintf(buff, "Received message: %s\n", comm_buff);
		syslog_append(buff);

		sprintf(comm_buff, "Hello back, this is server.");
		ret = sendto(data_socket, comm_buff, strlen(buff)+1, 0, (struct sockaddr *)&client_addr, *addr_len);

		if (ret <= 0)
		{
			perror("Failed to send to sub socket.");
			sprintf(buff, "Failed to send to sub socket.");
			syslog_append(buff);
		}
		else
		{
			perror("Sent reply to sub socket.");
			sprintf(buff, "Sent reply to sub socket.");
			syslog_append(buff);
		}
		sleep(1);
	}
}

void server_loop(void)
{
    char buff[64] = {0};

    int ret = listen(server_socket, 10);

    if (ret != 0)
    {
        perror("Failed to listen on socket.");
        sprintf(buff, "Failed to listen on socket.");
        syslog_append(buff);
	return;
    }

    ret = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

    if (ret < 0)
    {
        perror("Failed to accept on socket.");
        sprintf(buff, "Failed to accept on socket.");
        syslog_append(buff);
	return;
    }
    else
    {
	int data_socket = ret;
	connection_loop(data_socket, (struct sockaddr *)&client_addr, &addr_len);
    }
    sleep(1);
}

int main(void)
{
    char buff[64] = {0};
    syslog_init("Hub Intercom Server");
    initialize_signal_handler();
    sprintf(buff, "Starting Intercom Server, PID %u", getpid());
    syslog_append(buff);

    server_init();

    for(;;)
    {
        server_loop();
    }
}
