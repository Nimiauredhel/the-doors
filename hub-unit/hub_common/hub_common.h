#ifndef COMMON_H
#define COMMON_H

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
#include <sys/mman.h>

#include <signal.h>
#include <pthread.h>

#include <mqueue.h>
#include <semaphore.h>

#include "../../common/packet_defs.h"
#include "../../common/packet_utils.h"

#include "hub_queue.h"

#define CLIENTS_TO_DOORS_QUEUE_NAME "/mq_clients_to_doors"
#define DOORS_TO_CLIENTS_QUEUE_NAME "/mq_doors_to_clients"
#define MQ_MSG_SIZE_MAX (sizeof(DoorPacket_t) + DOOR_DATA_BYTES_SMALL)
#define MQ_MSG_COUNT_MAX (10)

#define DOOR_STATES_SHM_NAME "DOORS_DOOR_STATES_SHM"
#define CLIENT_STATES_SHM_NAME "DOORS_CLIENT_STATES_SHM"

#define DOOR_STATES_SEM_NAME "DOORS_DOOR_STATES_SEM"
#define CLIENT_STATES_SEM_NAME "DOORS_CLIENT_STATES_SEM"

#define HUB_MAX_DOOR_COUNT (32)
#define HUB_MAX_CLIENT_COUNT (128)

typedef struct HubDoorStates
{
    bool slot_used[HUB_MAX_CLIENT_COUNT];
    time_t last_seen[HUB_MAX_DOOR_COUNT];
    uint8_t i2c_addresses[HUB_MAX_DOOR_COUNT];
    char name[HUB_MAX_DOOR_COUNT][UNIT_NAME_MAX_LEN];
} HubDoorStates_t;

typedef struct HubClientStates
{
    bool slot_used[HUB_MAX_CLIENT_COUNT];
    time_t last_seen[HUB_MAX_CLIENT_COUNT];
    uint8_t mac_addresses[HUB_MAX_CLIENT_COUNT][6];
    char name[HUB_MAX_CLIENT_COUNT][UNIT_NAME_MAX_LEN];
} HubClientStates_t;

void syslog_init(char *self_label);
void syslog_append(char *msg);
void initialize_signal_handler(void);
void initialize_random_seed(void);
void signal_handler(int signum);
int random_range(int min, int max);
struct tm get_datetime(void);

#endif
