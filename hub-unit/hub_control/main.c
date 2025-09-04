#include "hub_common.h"

#define NUM_PROCESSES 2

typedef struct HubProcess
{
    pid_t pid;
    char *exe_name;
} HubProcess_t;

static const struct mq_attr hub_mq_attributes =
{
    .mq_flags = 0,
    .mq_maxmsg = MQ_MSG_COUNT_MAX,
    .mq_msgsize = MQ_MSG_SIZE_MAX,
    .mq_curmsgs = 0,
};

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

static void ipc_init(void)
{
    mqd_t mq_handle;

    mq_handle = mq_open(DOORS_TO_CLIENTS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (mq_handle < 0)
    {
        if (errno == EEXIST)
        {
            syslog_append("Queue 'doors to clients' already exists.");
        }
        else
        {
            perror("Failed to open or create 'doors to clients' queue");
            syslog_append("Failed to open or create 'doors to clients' queue");
            exit(EXIT_FAILURE);
        }
    }

    mq_close(mq_handle);

    mq_handle = mq_open(CLIENTS_TO_DOORS_QUEUE_NAME, O_CREAT | O_EXCL, 0666, &hub_mq_attributes);

    if (mq_handle < 0)
    {
        if (errno == EEXIST)
        {
            syslog_append("Queue 'clients to doors' already exists.");
        }
        else
        {
            perror("Failed to open or create 'clients to doors' queue");
            syslog_append("Failed to open or create 'clients to doors' queue");
            exit(EXIT_FAILURE);
        }
    }

    mq_close(mq_handle);
}

static void launch_process(uint8_t index)
{
    char log_buff[128] = {0};
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Failed to fork");
        syslog_append("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        int exec_ret = execl(processes[index].exe_name, processes[index].exe_name, NULL);

        perror("Failed to execute process");
        snprintf(log_buff, sizeof(log_buff), "Failed to execute process: %s", strerror(exec_ret));
        syslog_append(log_buff);
        exit(EXIT_FAILURE);
    }
    else
    {
        processes[index].pid = pid;
        snprintf(log_buff, sizeof(log_buff), "Successfully launched program %s with PID %d.", processes[index].exe_name, processes[index].pid);
        syslog_append(log_buff);
    }
}

static void control_loop(void)
{
    static const uint16_t delay_secs = 15;
    static uint64_t runtime_secs = 0;
    
    char log_buff[128] = {0};
    int status;
    int ret;

    for(;;)
    {
        sleep(delay_secs);
        runtime_secs += delay_secs;

        snprintf(log_buff, sizeof(log_buff), "Still alive, runtime: %lu seconds", runtime_secs);
        syslog_append(log_buff);

        for (int i = 0; i < NUM_PROCESSES; i++)
        {
            ret = waitpid(processes[i].pid, &status, WNOHANG);

            if (ret != 0)
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) stopped with signal %d - relaunching.", processes[i].exe_name, processes[i].pid, WSTOPSIG(status));
                syslog_append(log_buff);
                launch_process(i);
            }
            else
            {
                snprintf(log_buff, sizeof(log_buff), "Program %s (PID %d) still running.", processes[i].exe_name, processes[i].pid);
                syslog_append(log_buff);
            }
        }
    }
}

int main(void)
{
    syslog_init("DOORS Hub Control");
    initialize_signal_handler();

    // daemonize
    sleep(1);
    daemon_init();

    // initializee hub common resources (pipes, shared memory, etc.)
    ipc_init();

    // prepare the arguments to be passed to individual processes

    // launch all hub processes
    sleep(1);
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
	    launch_process(i);
    }

    // launch web server ?
    //system("sh init_web.sh");

    // begin loop checking that all processes are still running
    sleep(1);
    control_loop();

    return 1;
}
