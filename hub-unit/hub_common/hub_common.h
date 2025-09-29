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

#include "../../common/packet_defs.h"
#include "../../common/packet_utils.h"

#include "hub_queue.h"

#define HUB_MODULE_COUNT (5)
#define HUB_MODULE_NAME_MAX_LEN (32)

#define HUB_MAX_DOOR_COUNT (100)
#define HUB_MAX_CLIENT_COUNT (100)
#define HUB_MAX_LOG_COUNT (100)
#define HUB_MAX_LOG_MSG_LENGTH (128)

typedef enum HubModuleId
{
    HUB_MODULE_NONE = 0,
    HUB_MODULE_HUB_CONTROL = 1,
    HUB_MODULE_DOOR_MANAGER = 2,
    HUB_MODULE_INTERCOM_SERVER = 3,
    HUB_MODULE_WEB_SERVER = 4,
    HUB_MODULE_DATABASE_SERVICE = 5,
} HubModuleId_t;

typedef struct HubDoorStates
{
    uint8_t count;
    uint8_t id[HUB_MAX_DOOR_COUNT];
    time_t last_seen[HUB_MAX_DOOR_COUNT];
    char name[HUB_MAX_DOOR_COUNT][UNIT_NAME_MAX_LEN];
} HubDoorStates_t;

typedef struct HubClientStates
{
    bool slot_used[HUB_MAX_CLIENT_COUNT];
    time_t last_seen[HUB_MAX_CLIENT_COUNT];
    uint8_t mac_addresses[HUB_MAX_CLIENT_COUNT][6];
    char name[HUB_MAX_CLIENT_COUNT][UNIT_NAME_MAX_LEN];
} HubClientStates_t;

typedef struct HubLogRing
{
    uint16_t head;
    HubModuleId_t module_ids[HUB_MAX_LOG_COUNT];
    struct tm timestamps[HUB_MAX_LOG_COUNT];
    char logs[HUB_MAX_LOG_COUNT][HUB_MAX_LOG_MSG_LENGTH];
} HubLogRing_t;

/**
 * Global flag set by OS termination signals
 * and polled by functions to allow graceful termination.
 */
extern bool should_terminate;

void set_module_id(HubModuleId_t module_id);
const char* get_module_label(HubModuleId_t module_id);
void log_init(bool init_shm);
void log_deinit(void);
void log_append(char *msg);
void initialize_signal_handler(void);
void initialize_random_seed(void);
void signal_handler(int signum);
int random_range(int min, int max);
struct tm get_datetime(void);

#endif
