#include "hub_common.h"
#include "intercom_server_common.h"

int main(void)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    log_init("Intercom-Server", false);
    initialize_signal_handler();
    snprintf(log_buff, sizeof(log_buff), "Starting process with PID %u", getpid());
    log_append(log_buff);

    server_start();

    log_deinit();

    return 0;
}
