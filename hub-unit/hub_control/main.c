#include <sys/types.h>
#include <sys/wait.h>

#include "../hub_common.h"

#include "packet_defs.h"
#include "packet_utils.h"
#include "i2c_register_defs.h"

typedef struct HubProcess
{
    pid_t pid;
    char *exe_name;
} HubProcess_t;

static const uint16_t check_interval_sec = 5;

static int pid_pipe[2] = {0};

static HubProcess_t processes[] =
{
    {-1, "door_manager"},
 //   {-1, "intercom_server"},
 //   {-1, "db_service"},
};

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

static void launch_process(uint8_t index)
{
    char *args[] = {processes[0].exe_name, NULL};
    pid_t pid = fork();

    if (pid < 0)
    {
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
        int exec_ret = execv(processes[index].exe_name, args);
        if (exec_ret != 0)
        {
            perror("Failed to execute process");
            exit(EXIT_FAILURE);
        }
        printf("This print should not run\n");
    }
    else
    {
        /*
        waitpid(pid, NULL, 0);
        printf("Successfully waited on PID %d.\n", pid);
        processes[0].pid = read_pid_pipe();
        */
        processes[0].pid = pid;
        printf("Successfully launched program %s with PID %d.\n", processes[index].exe_name, processes[index].pid);
    }

}

int main(void)
{
    // daemonize
    daemon(1,1);

    // initializee hub common resources (pipes, shared memory, etc.)
    int ret = pipe(pid_pipe);

    // prepare the arguments to be passed to individual processes

    // launch all hub processes
    launch_process(0);

    // begin loop checking that all processes are still running
    for(;;)
    {
        int status;
        int ret = waitpid(processes[0].pid, &status, WNOHANG);

        if (ret != 0)
        {
            printf("Program %s (PID %d) seems to have been terminated - relaunching.\n", processes[0].exe_name, processes[0].pid);
            launch_process(0);
        }
        else
        {
            printf("Program %s still running with code %d.\n", processes[0].exe_name, status);
        }

        sleep(check_interval_sec);
    }
}
