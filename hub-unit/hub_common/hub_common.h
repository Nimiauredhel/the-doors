#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>

#include "../../common/packet_defs.h"
#include "../../common/packet_utils.h"

#define CLIENTS_TO_DOORS_SHM_SEM "324"
#define DOORS_TO_CLIENTS_SHM_SEM "423"
#define CLIENTS_TO_DOORS_SHM_KEY 324
#define DOORS_TO_CLIENTS_SHM_KEY 423
#define SHM_PACKET_EXTRA_DATA_BYTES 160000

typedef enum ShmState
{
    SHMSTATE_NONE  = 0,
    SHMSTATE_DIRTY = 1,
    SHMSTATE_CLEAN = 2,
} ShmState_t;

typedef struct HubShmLayout
{
    ShmState_t state;
    DoorPacket_t content;
    uint8_t data[SHM_PACKET_EXTRA_DATA_BYTES];
} HubShmLayout_t;

void syslog_init(char *self_label);
void syslog_append(char *msg);
void initialize_signal_handler(void);
void initialize_random_seed(void);
void signal_handler(int signum);
int random_range(int min, int max);
struct tm get_datetime(void);

#endif
