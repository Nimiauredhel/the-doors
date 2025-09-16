#include "hub_control_processes.h"
#include "hub_common.h"

#define NUM_PROCESSES (3)

typedef struct HubProcess
{
    pid_t pid;
    char *exe_name;
} HubProcess_t;

static HubProcess_t processes[] =
{
    {-1, "/usr/bin/doors_hub/door_manager"},
    {-1, "/usr/bin/doors_hub/intercom_server"},
    {-1, "/usr/bin/doors_hub/db_service"},
};

static void processes_launch_one(uint8_t index)
{
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
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

void processes_terminate_all(void)
{
    static const struct timespec termination_delay =
    {
        .tv_nsec = 500000000,
        .tv_sec = 0,
    };

    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    int status = 0;
    int child_ret = 0;
    bool sent_sigterm = false;

    log_append("Terminating child processes.");
    
    // first try SIGTERM
    for (uint8_t i = 0; i < NUM_PROCESSES; i++)
    {
        if (processes[i].pid <= 0)
        {
            snprintf(log_buff, sizeof(log_buff), "ABNORMAL: Program %s registered as PID %d, skipping SIGTERM round.", processes[i].exe_name, processes[i].pid);
            log_append(log_buff);
            continue;
        }

        child_ret = waitpid(processes[i].pid, &status, WNOHANG);

        if (child_ret != 0)
        {
            snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) has already stopped with signal %d.", processes[i].exe_name, processes[i].pid, WSTOPSIG(status));
            log_append(log_buff);
        }
        else
        {
            snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) is still running - sending SIGTERM.", processes[i].exe_name, processes[i].pid);
            log_append(log_buff);
            kill(processes[i].pid, SIGTERM);
            sent_sigterm = true;
            // add a small delay per signal sent
            nanosleep(&termination_delay, NULL);
        }
    }

    /**
     * If SIGTERM had to be sent, wait 1 second,
     * then initiate a second round of checkups
     * and send SIGKILL to any child processes that linger.
     **/
    if(sent_sigterm)
    {
        sleep(1);

        for (uint8_t i = 0; i < NUM_PROCESSES; i++)
        {
            if (processes[i].pid <= 0)
            {
                snprintf(log_buff, sizeof(log_buff), "ABNORMAL: Program %s registered as PID %d, skipping SIGKILL round.", processes[i].exe_name, processes[i].pid);
                log_append(log_buff);
                continue;
            }

            child_ret = waitpid(processes[i].pid, &status, WNOHANG);

            if (child_ret != 0)
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) has already stopped with signal %d.", processes[i].exe_name, processes[i].pid, WSTOPSIG(status));
                log_append(log_buff);
            }
            else
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) is still running after SIGTERM - sending SIGKILL.", processes[i].exe_name, processes[i].pid);
                log_append(log_buff);
                kill(processes[i].pid, SIGKILL);
            }
        }
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
    static const useconds_t control_loop_delay_secs = 15;
    static time_t run_time_s = 0;
    
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    int status = 0;
    int this_ret = 0;
    int child_ret = 0;

    time_t launch_time_s = time(NULL);

    sleep(control_loop_delay_secs);

    // TODO: add termination conditions (including when executable has been modified)
    // TODO: implement graceful termination 
    // TODO: meaningful return value
    while(!should_terminate)
    {
        run_time_s = time(NULL) - launch_time_s;

        snprintf(log_buff, sizeof(log_buff), "Runtime: %lus, initiating child process checkup.", run_time_s);
        log_append(log_buff);

        for (uint8_t i = 0; i < NUM_PROCESSES; i++)
        {
            if (should_terminate) break;

            if (processes[i].pid <= 0)
            {
                snprintf(log_buff, sizeof(log_buff), "ABNORMAL: Program %s should have been launched but is registered as PID %d. Skipping.", processes[i].exe_name, processes[i].pid);
                log_append(log_buff);
                continue;
            }

            child_ret = waitpid(processes[i].pid, &status, WNOHANG);

            if (should_terminate) break;

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

        sleep(control_loop_delay_secs);
    }

    return this_ret;
}
