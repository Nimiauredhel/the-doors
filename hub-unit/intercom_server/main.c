#include "hub_common.h"
#include "intercom_server_common.h"

int main(void)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    set_module_id(HUB_MODULE_INTERCOM_SERVER);
    log_init(false);
    initialize_signal_handler();
    snprintf(log_buff, sizeof(log_buff), "Starting process with PID %u", getpid());
    log_append(log_buff);

    server_start();

    log_deinit();

    return 0;
}
