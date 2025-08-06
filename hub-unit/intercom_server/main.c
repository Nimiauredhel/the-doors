#include "hub_common.h"
#include "server.h"

int main(void)
{
    char log_buff[128] = {0};
    syslog_init("DOORS Intercom Server");
    initialize_signal_handler();
    snprintf(log_buff, sizeof(log_buff), "Starting Intercom Server, PID %u", getpid());
    syslog_append(log_buff);

    server_start();
}
