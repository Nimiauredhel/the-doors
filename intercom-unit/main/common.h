#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

/**
 * Global flag set by OS termination signals
 * and polled by functions to allow graceful termination.
 */
extern volatile bool should_terminate;

int random_range(int min, int max);
float seconds_since_clock(struct timespec *start_clock);

#endif
