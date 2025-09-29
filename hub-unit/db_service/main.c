#include "hub_common.h"

#include "db_service_ipc.h"
#include "db_service_db.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};

    set_module_id(HUB_MODULE_DATABASE_SERVICE);
    log_init(false);
    snprintf(log_buff, sizeof(log_buff), "Starting process with PID %u", getpid());
    log_append(log_buff);
    initialize_signal_handler();

    ipc_init();
    db_init();

    // TODO: implement ipc_out and run in a second thread
    ipc_in_task(NULL);

    ipc_deinit();
    db_deinit();

    snprintf(log_buff, sizeof(log_buff), "Ending process with PID %u", getpid());
    log_append(log_buff);

    log_deinit();

    return 0;
}
