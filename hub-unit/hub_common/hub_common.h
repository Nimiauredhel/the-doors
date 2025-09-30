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
#include "../../common/i2c_register_defs.h"

#include "hub_queue.h"

#define HUB_MODULE_COUNT (5)
#define HUB_MODULE_NAME_MAX_LEN (32)

#define HUB_MAX_DOOR_COUNT (I2C_ADDRESS_COUNT)
#define HUB_MAX_INTERCOM_COUNT (128)
#define HUB_MAX_LOG_COUNT (1024)
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

// TODO: move this to the project common source and use in Door Unit instead of 'DoorFlags_t'
// TODO: more flags will be added and implemented by Door Unit, such as: has admin, interaction ongoing
typedef enum DoorStateFlags
{
	DOOR_FLAG_NONE       = 0x00,
	DOOR_FLAG_CLOSED     = 0x01, // 0b..00 = open, 0b..01 = closed
	DOOR_FLAG_TRANSITION = 0x02, // 0b..10 = opening, 0b..11 = closing
} DoorStateFlags_t;

// TODO: decide if a similar 'IntercomStateFlags_t' struct could be useful

typedef struct HubDoorStates
{
    uint16_t count;
    time_t last_seen[HUB_MAX_DOOR_COUNT];
    DoorStateFlags_t flags[HUB_MAX_DOOR_COUNT];
    char name[HUB_MAX_DOOR_COUNT][UNIT_NAME_MAX_LEN];
} HubDoorStates_t;

typedef struct HubIntercomStates
{
    uint16_t count;
    time_t last_seen[HUB_MAX_INTERCOM_COUNT];
    uint8_t mac_addresses[HUB_MAX_INTERCOM_COUNT][6];
    char name[HUB_MAX_INTERCOM_COUNT][UNIT_NAME_MAX_LEN];
} HubIntercomStates_t;

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
