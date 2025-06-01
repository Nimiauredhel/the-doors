#include "hub_common.h"

/**
 * The random_range() function uses this to determine
 * whether rand() was already seeded or not.
 */
static bool random_was_seeded = false;

void syslog_init(char *self_label)
{
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(self_label, LOG_CONS | LOG_PERROR, LOG_USER);
}

void syslog_append(char *msg)
{
    syslog(LOG_INFO, "%s", msg);
}

/**
 * Hooks up OS signals to our custom handler.
 */
void initialize_signal_handler(void)
{
    syslog_append("Initializing signal handler");

    struct sigaction new_sigaction;
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
