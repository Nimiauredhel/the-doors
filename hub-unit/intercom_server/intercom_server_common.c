#include "intercom_server_common.h"

#include "intercom_server_ipc.h"
#include "intercom_server_listener.h"

static pthread_t ipc_out_thread_handle;
static pthread_t ipc_in_thread_handle;

static void server_init(void)
{
    ipc_init();
    listener_init();
}

void server_start(void)
{
    server_init();

    pthread_create(&ipc_out_thread_handle, NULL, ipc_out_task, NULL);
    pthread_create(&ipc_in_thread_handle, NULL, ipc_in_task, NULL);

    // not creating a new thread since main thread is doing nothing
    listener_task(NULL);

    for(;;);
}
