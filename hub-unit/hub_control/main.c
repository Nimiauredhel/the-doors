#include "hub_common.h"
#include "hub_control_daemon.h"
#include "hub_control_processes.h"

int main(void)
{
    log_init("Hub-Control", true);
    initialize_signal_handler();

    // daemonize
    daemon_init();

    // launch all hub processes
    processes_launch_all();

    // begin loop checking that all processes are still running
    return processes_control_loop();
}
