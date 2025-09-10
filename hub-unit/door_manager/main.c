#include "door_manager_common.h"
#include "door_manager_ipc.h"
#include "door_manager_i2c.h"

static pthread_t i2c_thread;
static pthread_t ipc_out_thread;
/// not creating a third thread since main is currently doing nothing
/// TODO: find use for another thread or clean this up
//static pthread_t ipc_in_thread;

int main(void)
{
    char log_buff[128] = {0};

    log_init("Door-Manager");
    snprintf(log_buff, sizeof(log_buff), "Starting process with PID %u", getpid());
    log_append(log_buff);
    initialize_signal_handler();
    door_manager_init();

    if (!should_terminate) ipc_init();
    if (!should_terminate) i2c_init();

    if (!should_terminate)
    {
        pthread_create(&i2c_thread, NULL, i2c_task, NULL);
        pthread_create(&ipc_out_thread, NULL, ipc_out_task, NULL);

        /// not creating a third thread since main is currently doing nothing
        /// TODO: find use for another thread or clean this up
        ipc_in_task(NULL);

        while (!should_terminate);
    }

    door_manager_deinit();
    return 0;
}
