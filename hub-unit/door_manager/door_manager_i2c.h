#ifndef DOOR_MANAGER_I2C_H
#define DOOR_MANAGER_I2C_H

#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "i2c_register_defs.h"
#include "door_manager_common.h"
#include "door_manager_ipc.h"

void i2c_forward_request(DoorPacket_t *request);
void i2c_terminate(void);
void* i2c_task(void *arg);
void i2c_init(void);

#endif
