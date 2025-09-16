#include "hub_common.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    log_init("Database-Service", false);
    snprintf(log_buff, sizeof(log_buff), "Starting process with PID %u", getpid());
    log_append(log_buff);
    initialize_signal_handler();

    while (!should_terminate)
    {
        sleep(1);
    }

    snprintf(log_buff, sizeof(log_buff), "Ending process with PID %u", getpid());
    log_append(log_buff);

    log_deinit();

    return 0;
}
