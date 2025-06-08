#include "hub_common.h"
#include "server.h"

int main(void)
{
    char buff[64] = {0};
    syslog_init("Hub Intercom Server");
    initialize_signal_handler();
    sprintf(buff, "Starting Intercom Server, PID %u", getpid());
    syslog_append(buff);

    server_start();
}
