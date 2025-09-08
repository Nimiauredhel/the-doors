#include "hub_control_processes.h"
#include "hub_control_ipc.h"
#include "hub_common.h"

#define NUM_PROCESSES 2

typedef struct HubProcess
{
    pid_t pid;
    char *exe_name;
} HubProcess_t;

static HubProcess_t processes[] =
{
    {-1, "/usr/bin/doors_hub/door_manager"},
    {-1, "/usr/bin/doors_hub/intercom_server"},
 //   {-1, "/usr/bin/doors_hub/db_service"},
};

static void processes_launch_one(uint8_t index)
{
    char log_buff[128] = {0};
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Failed to fork");
        log_append("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        int exec_ret = execl(processes[index].exe_name, processes[index].exe_name, NULL);

        perror("Failed to execute process");
        snprintf(log_buff, sizeof(log_buff), "Failed to execute process: %s", strerror(exec_ret));
        log_append(log_buff);
        exit(EXIT_FAILURE);
    }
    else
    {
        processes[index].pid = pid;
        snprintf(log_buff, sizeof(log_buff), "Successfully launched program %s with PID %d.", processes[index].exe_name, processes[index].pid);
        log_append(log_buff);
    }
}

void processes_launch_all(void)
{
    for (uint8_t i = 0; i < NUM_PROCESSES; i++)
    {
	    processes_launch_one(i);
    }
}

int processes_control_loop(void)
{
    static const uint16_t delay_secs = 10;
    static time_t run_time_s = 0;
    
    char log_buff[128] = {0};
    int status = 0;
    int this_ret = 0;
    int child_ret = 0;

    time_t launch_time_s = time(NULL);

    /// TODO: add termination conditions (including when executable has been modified)
    /// TODO: implement graceful termination 
    /// TODO: meaningful return value
    while(!should_terminate)
    {
        sleep(delay_secs);
        run_time_s = time(NULL) - launch_time_s;

        snprintf(log_buff, sizeof(log_buff), "Still alive, runtime: %lus", run_time_s);
        log_append(log_buff);

        for (uint8_t i = 0; i < NUM_PROCESSES; i++)
        {
            child_ret = waitpid(processes[i].pid, &status, WNOHANG);

            if (child_ret != 0)
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) stopped with signal %d - relaunching.", processes[i].exe_name, processes[i].pid, WSTOPSIG(status));
                log_append(log_buff);
                processes_launch_one(i);
            }
            else
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) still running.", processes[i].exe_name, processes[i].pid);
                log_append(log_buff);
            }
        }
    }

    /// TODO: terminate child processes before exiting
    return this_ret;
}
