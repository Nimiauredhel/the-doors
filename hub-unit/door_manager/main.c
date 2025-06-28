#include "door_manager_common.h"
#include "door_manager_ipc.h"
#include "door_manager_i2c.h"

static pthread_t i2c_thread;
static pthread_t ipc_thread;

static void terminate(int ret)
{
    ipc_terminate();
    i2c_terminate();
    common_terminate();

    exit(ret);
}

int main(void)
{
    char buff[64] = {0};

    syslog_init("Hub Door Manager");
    initialize_signal_handler();
    sprintf(buff, "Starting Door Manager, PID %u", getpid());
    syslog_append(buff);
    common_init();
    ipc_init();
    i2c_init();
    pthread_create(&i2c_thread, NULL, i2c_task, NULL);
    pthread_create(&ipc_thread, NULL, ipc_task, NULL);

    for(;;);
}
