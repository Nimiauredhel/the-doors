#include "common.h"

volatile bool should_terminate = false;

/**
 * The random_range() function uses this to determine
 * whether rand() was already seeded or not.
 */
static bool random_was_seeded = false;

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

/**
 * Returns the seconds elapsed since a given clock value.
 * Used for timing operations!
 */
float seconds_since_clock(struct timespec *start_clock)
{
    /*struct timespec now_clock;
    clock_gettime(CLOCK_MONOTONIC, &now_clock);
    float elapsed_float = (now_clock.tv_nsec - start_clock->tv_nsec) / 1000000000.0;
    elapsed_float += (now_clock.tv_sec - start_clock->tv_sec);
    return elapsed_float;
    */
	return 0.0f;
}
