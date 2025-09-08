#include "hub_control_daemon.h"
#include "hub_common.h"

void daemon_init(void)
{
    log_append("Daemonizing!");

    int ret = daemon(1, 0);

    if (ret < 0)
    {
        perror("Failed to Daemonize");
        log_append("Failed to Daemonize, Terminating");
        exit(EXIT_FAILURE);
    }

    log_append("Successfully Daemonized");
}
