#include "hub_common.h"

/**
 * The random_range() function uses this to determine
 * whether rand() was already seeded or not.
 */
static bool random_was_seeded = false;

static char process_label[32] = {0};

void syslog_init(char *self_label)
{
    char syslog_label[40] = {0};
    snprintf(process_label, sizeof(process_label), "%s", self_label);
    snprintf(syslog_label, sizeof(syslog_label), "DOORS %s", self_label);

    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(syslog_label, LOG_CONS | LOG_PERROR, LOG_USER);
}

void syslog_append(char *msg)
{
    syslog(LOG_INFO, "%s", msg);

    struct tm now_dt = get_datetime();

    FILE *file = fopen("site/logs.txt", "a");

    /// only retry once
    /// TODO: implement an internal log queue to handle this properly
    if (file == NULL)
    {
        usleep(1000);
        file = fopen("site/logs.txt", "a");
    }

    if (file != NULL)
    {
        fprintf(file, "[%02u:%02u:%02u][%s]%s\n", now_dt.tm_hour, now_dt.tm_min, now_dt.tm_sec, process_label, msg);
        fclose(file);
    } 
}

/**
 * Hooks up OS signals to our custom handler.
 */
void initialize_signal_handler(void)
{
    syslog_append("Initializing signal handler");

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
            syslog_append("Received signal SIGHUP");
            break;
        case SIGINT:
            syslog_append("Received signal SIGINT");
            exit(EXIT_SUCCESS);
            break;
        case SIGTERM:
            syslog_append("Received signal SIGTERM");
            exit(EXIT_SUCCESS);
            break;
        default:
            syslog_append("Received signal Unknown");
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
