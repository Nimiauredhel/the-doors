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

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);

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
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    client_addr.sin_family = AF_INET;

    int bind_result = bind(server_socket, (struct sockaddr *)&(server_addr), sizeof(server_addr));

    if (bind_result < 0)
    {
        sprintf(buff, "Could not bind requests socket");
        syslog_append(buff);
        perror("Could not bind requests socket");
        exit(EXIT_FAILURE);
    }

    sprintf(buff, "Successfully bound socket.");
    syslog_append(buff);
}

void server_loop(void)
{
    listen(server_socket, 10);
    accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
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
