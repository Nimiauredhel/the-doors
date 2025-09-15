#include "hub_common.h"
#include "hub_control_daemon.h"
#include "hub_control_ipc.h"
#include "hub_control_processes.h"

int main(void)
{
    log_init("Hub-Control", true);
    initialize_signal_handler();

    // daemonize
    daemon_init();

    // initializee hub common resources (pipes, shared memory, etc.)
    // TODO: rethink the role of this call
    ipc_init();

    // prepare the arguments to be passed to individual processes
    // TODO: rethink the imaginary role of this non-existent call

    // launch all hub processes
    processes_launch_all();

    // begin loop checking that all processes are still running
    return processes_control_loop();
}
