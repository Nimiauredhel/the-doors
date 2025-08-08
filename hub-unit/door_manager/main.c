#include "door_manager_common.h"
#include "door_manager_ipc.h"
#include "door_manager_i2c.h"

static pthread_t i2c_thread;
static pthread_t ipc_out_thread;

int main(void)
{
    char log_buff[128] = {0};

    syslog_init("DOORS Door Manager");
    snprintf(log_buff, sizeof(log_buff), "Starting Door Manager, PID %u", getpid());
    syslog_append(log_buff);
    initialize_signal_handler();
    common_init();
    ipc_init();
    i2c_init();
    pthread_create(&i2c_thread, NULL, i2c_task, NULL);
    pthread_create(&ipc_out_thread, NULL, ipc_out_task, NULL);
    /// not creating a new thread since main is currently doing nothing
    ipc_in_task(NULL);

    for(;;);
}
