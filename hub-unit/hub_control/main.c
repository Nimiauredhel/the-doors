#include <sys/types.h>
#include <sys/wait.h>

#include "hub_common.h"

#include "packet_defs.h"
#include "packet_utils.h"
#include "i2c_register_defs.h"

#define NUM_PROCESSES 2

typedef struct HubProcess
{
    pid_t pid;
    char *exe_name;
} HubProcess_t;

static const uint16_t check_interval_sec = 5;

//static int pid_pipe[2] = {0};

static HubProcess_t processes[] =
{
    {-1, "/usr/bin/doors_hub/door_manager"},
    {-1, "/usr/bin/doors_hub/intercom_server"},
 //   {-1, "/usr/bin/doors_hub/db_service"},
};


static void daemon_init(void)
{
    syslog_append("Daemonizing!");

    int ret = daemon(1, 0);

    if (ret < 0)
    {
        perror("Failed to Daemonize");
        syslog_append("Failed to Daemonize, Terminating");
        exit(EXIT_FAILURE);
    }

    syslog_append("Successfully Daemonized");
}
/*
static int read_pid_pipe(void)
{
    pid_t ret_pid = -1;
    read(pid_pipe[0], &ret_pid, sizeof(pid_t));
    return ret_pid;
}

static void write_pid_pipe(pid_t pid)
{
    write(pid_pipe[1], &pid, sizeof(pid_t));
}
*/

static void launch_process(uint8_t index)
{
    char buff[128] = {0};
    pid_t pid = fork();

    if (pid < 0)
    {
        syslog_append("Failed to fork");
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        /*
        printf("Child process here with PID %u\n", getpid());
        int daemon_ret = daemon(1, 1);
        if (daemon_ret != 0)
        {
            perror("Failed to daemonize");
            exit(EXIT_FAILURE);
        }
        write_pid_pipe(getpid());
        printf("Child process daemonized with PID %u\n", getpid());
        */
        int exec_ret = execl(processes[index].exe_name, processes[index].exe_name, NULL);
        sprintf(buff, "Failed to execute process: %s", strerror(exec_ret));
        syslog_append(buff);
        perror("Failed to execute process");
        exit(EXIT_FAILURE);
    }
    else
    {
        /*
        waitpid(pid, NULL, 0);
        printf("Successfully waited on PID %d.\n", pid);
        processes[0].pid = read_pid_pipe();
        */
        processes[index].pid = pid;
        sprintf(buff, "Successfully launched program %s with PID %d.\n", processes[index].exe_name, processes[index].pid);
        syslog_append(buff);
    }
}

static void control_loop(void)
{
    static const uint16_t delay_secs = 5;
    static uint64_t runtime_secs = 0;
    
    char syslog_buff[128] = {0};
    int status;
    int ret;

    for(;;)
    {
        sleep(delay_secs);
        runtime_secs += delay_secs;

        snprintf(syslog_buff, 128, "Still alive, runtime: %lu seconds", runtime_secs);
        syslog_append(syslog_buff);

	for (int i = 0; i < NUM_PROCESSES; i++)
	{
		ret = waitpid(processes[i].pid, &status, WNOHANG);

		if (ret != 0)
		{
		    snprintf(syslog_buff, 128, "Program %s (PID %d) seems to have been terminated - relaunching.\n", processes[i].exe_name, processes[i].pid);
		    syslog_append(syslog_buff);
		    launch_process(i);
		}
		else
		{
		    snprintf(syslog_buff, 128, "Program %s still running with code %d.\n", processes[i].exe_name, status);
		    syslog_append(syslog_buff);
		}
	}
    }
}

int main(void)
{
    syslog_init("Hub Control");
    initialize_signal_handler();

    // daemonize
    sleep(1);
    daemon_init();

    // initializee hub common resources (pipes, shared memory, etc.)
    //int ret = pipe(pid_pipe);

    // prepare the arguments to be passed to individual processes

    // launch all hub processes
    sleep(1);
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
	    launch_process(i);
    }

    // begin loop checking that all processes are still running
    sleep(1);
    control_loop();

    return 1;
}
