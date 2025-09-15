#include "hub_common.h"
#include "hub_common_ipc.h"

static const char *site_dir = "site";
static const char *logs_dir = "logs";
static const char *common_log_path = "logs/common.txt";

/**
 * Global flag set by OS termination signals
 * and polled by functions to allow graceful termination.
 */
bool should_terminate = false;

/**
 * The random_range() function uses this to determine
 * whether rand() was already seeded or not.
 */
static bool random_was_seeded = false;

static char process_label[32] = {0};
static char process_log_path[40] = {0};

static bool log_sys_initialized = false;
static bool log_shm_initialized = false;

void log_init(char *self_label, bool init_shm)
{
    char syslog_label[40] = {0};
    snprintf(process_label, sizeof(process_label), "%s", self_label);
    snprintf(syslog_label, sizeof(syslog_label), "DOORS-%s", self_label);

    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(syslog_label, LOG_CONS | LOG_PERROR, LOG_USER);
    log_sys_initialized = true;

    mkdir(logs_dir, 0777);
    snprintf(process_log_path, sizeof(process_log_path), "%s/%s.txt", logs_dir, self_label);

    // *** clear process log file if exists ***
    FILE *file = fopen(process_log_path, "w");

    // retry once
    // TODO: loop with retry counter
    if (file == NULL)
    {
        usleep(1000);
        file = fopen(process_log_path, "w");
    }

    if (file != NULL)
    {
        fprintf(file, "Cleared.\n");
        fclose(file);
    } 

    // *** initialize handle to common log shm
    log_shm_initialized = ipc_init_hub_log_ptrs(init_shm);
}

void log_append(char *msg)
{
    if (log_sys_initialized)
    {
        syslog(LOG_INFO, "%s", msg);
    }

    // preparing the txt log string
    char log_buff[HUB_MAX_LOG_MSG_LENGTH] = {0};
    struct tm now_dt = get_datetime();

    snprintf(log_buff, sizeof(log_buff), "[%s]%s\n", process_label, msg);

    char formatted_log_buff[192] = {0};
    FILE *file = NULL;

    // *** write to process specific log txt file ***
    snprintf(formatted_log_buff, sizeof(formatted_log_buff), "[%02u:%02u:%02u]%s\n", now_dt.tm_hour, now_dt.tm_min, now_dt.tm_sec, log_buff);
    file = fopen(process_log_path, "a");

    // only retry once
    // TODO: implement an internal log queue to handle this properly
    if (file == NULL)
    {
        usleep(1000);
        file = fopen(process_log_path, "a");
    }

    if (file != NULL)
    {
        fprintf(file, "%s", formatted_log_buff);
        fclose(file);
    } 

    if (!log_shm_initialized) return;

    // *** write to common log ring buffer ***
    HubLogRing_t *hub_log_ptr = ipc_acquire_hub_log_ptr();
    if (hub_log_ptr->head == 0 && hub_log_ptr->logs[0][0] == '\0')
    {
        // first write, no need to increment
    }
    else
    {
        hub_log_ptr->head = (hub_log_ptr->head + 1) % HUB_MAX_LOG_COUNT;
    }

    hub_log_ptr->timestamps[hub_log_ptr->head] = now_dt;
    strncpy(hub_log_ptr->logs[hub_log_ptr->head], log_buff, HUB_MAX_LOG_MSG_LENGTH);
    hub_log_ptr->timestamps[hub_log_ptr->head] = now_dt;

    // *** overwrite common log txt file from ring buffer, int descending order ***
    file = fopen(common_log_path, "w");

    // only retry once
    // TODO: implement an internal log queue to handle this properly
    if (file == NULL)
    {
        usleep(1000);
        file = fopen(common_log_path, "w");
    }

    if (file != NULL)
    {
        for (uint16_t i = 0; i < HUB_MAX_LOG_COUNT; i++)
        {
            uint16_t read_pos = (hub_log_ptr->head + (HUB_MAX_LOG_COUNT - i)) % HUB_MAX_LOG_COUNT;

            if (hub_log_ptr->logs[read_pos][0] == '\0')
            {
                // reached empty slot, stop here
                break;
            }

            fprintf(file, "[%u][%02u:%02u:%02u]%s\n",
                    read_pos,
                    hub_log_ptr->timestamps[read_pos].tm_hour,
                    hub_log_ptr->timestamps[read_pos].tm_min,
                    hub_log_ptr->timestamps[read_pos].tm_sec,
                    hub_log_ptr->logs[read_pos]);
        }

        fclose(file);
    } 

    // release common log ring buffer
    ipc_release_hub_log_ptr();
}

/**
 * Hooks up OS signals to our custom handler.
 */
void initialize_signal_handler(void)
{
    log_append("Initializing signal handler");

    struct sigaction new_sigaction = {0};
    new_sigaction.sa_handler = signal_handler;

    sigaction(SIGHUP, &new_sigaction, NULL);
    sigaction(SIGTERM, &new_sigaction, NULL);
    sigaction(SIGINT, &new_sigaction, NULL);
}

// calling random_range once to ensure that random is seeded
void initialize_random_seed(void)
{
    usleep(random_range(1234, 5678));
}

/**
 * Handles selected OS signals.
 * Termination signals are caught to set the should_terminate flag
 * which signals running functions that they should attempt graceful termination.
 */
void signal_handler(int signum)
{
    // TODO: use global bool to request processes to terminate gracefully
    switch (signum)
    {
        case SIGHUP:
            log_append("Received signal SIGHUP");
            break;
        case SIGINT:
            log_append("Received signal SIGINT");
            should_terminate = true;
            break;
        case SIGTERM:
            log_append("Received signal SIGTERM");
            should_terminate = true;
            break;
        default:
            log_append("Received signal Unknown");
            break;
    }
}

/**
 * Returns a random int between min and max.
 * Also checks if rand() was already seeded.
 */
int random_range(int min, int max)
{
    if (!random_was_seeded)
    {
        srand(time(NULL) + getpid());
        random_was_seeded = true;
    }

    int random_number = rand();
    random_number = (random_number % (max - min + 1)) + min;
    return random_number;
}

struct tm get_datetime(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    return *timeinfo;
}
