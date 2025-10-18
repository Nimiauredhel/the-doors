/*
 * door_unit_main.c
 *
 *  Created on: Oct 16, 2025
 *      Author: mickey
 */

#include <string.h>
#include <stdint.h>

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lptim.h"

#include "persistence.h"
#include "door_control.h"
#include "door_sensor.h"
#include "user_interface.h"
#include "hub_comms.h"

#define DOOR_TASKS_STACK_SIZE (1024)
#define DOOR_TASKS_COUNT (5)
#define DOOR_TASKS_NAME_MAX_LEN (configMAX_TASK_NAME_LEN)

typedef enum DoorUnitTaskIndex
{
	DOORTASK_NONE = -1,
	DOORTASK_UI = 0,
	DOORTASK_DOORCONTROL = 1,
	DOORTASK_HUBCOMMS = 2,
	DOORTASK_DOORSENSOR = 3,
	DOORTASK_DISPLAY = 4,
	DOORTASK_COUNT = DOOR_TASKS_COUNT,
} DoorUnitTaskIndex_t;

typedef void(*TaskFunction_t)(void*);

void StartUserInterfaceTask(void *argument);
void StartDoorOpsTask(void *argument);
void StartCommsTask(void *argument);
void StartDoorSensorTask(void *argument);
void StartDisplayTask(void *argument);

static const char task_names[DOOR_TASKS_COUNT][DOOR_TASKS_NAME_MAX_LEN] =
{
    "UITask",
    "DoorControlTask",
    "HubCommsTask",
    "DoorSensorTask",
    "DisplayTask",
};

static const TaskFunction_t task_functions[DOOR_TASKS_COUNT] =
{
    StartUserInterfaceTask,
    StartDoorOpsTask,
    StartCommsTask,
    StartDoorSensorTask,
    StartDisplayTask,
};

static const osPriority_t task_priorities[DOOR_TASKS_COUNT] =
{
    osPriorityNormal,
    osPriorityRealtime,
    osPriorityNormal,
    osPriorityRealtime,
    osPriorityNormal,
};

static osThreadId_t task_handles[DOOR_TASKS_COUNT] = {0};
static uint32_t task_buffers[DOOR_TASKS_COUNT][DOOR_TASKS_STACK_SIZE] = {0};
static StaticTask_t task_control_blocks[DOOR_TASKS_COUNT] = {0};
static osThreadAttr_t task_attributes[DOOR_TASKS_COUNT] = {0};

static void init_modules(void)
{
	serial_uart_initialize();
	event_log_initialize();
	event_log_append_report_minimal(PACKET_REPORT_FRESH_BOOT);

	door_control_init();
	// the Low Power Timer is used for the door auto-closing countdown
	// TODO: move this to the door control file ?
	HAL_LPTIM_Counter_Start_IT(&hlptim1, 28125);
	comms_init();
    interface_init();
	display_init();
}

static void create_tasks(void)
{
	for (uint8_t i = 0; i < DOOR_TASKS_COUNT; i++)
	{
		task_attributes[i].name = task_names[i];
		task_attributes[i].cb_mem = &task_control_blocks[i];
		task_attributes[i].cb_size = sizeof(StaticTask_t);
		task_attributes[i].stack_mem = &task_buffers[i];
		task_attributes[i].stack_size = DOOR_TASKS_STACK_SIZE;
		task_attributes[i].priority = task_priorities[i];
		task_handles[i] = osThreadNew(task_functions[i], NULL, &task_attributes[i]);
	}
}

void door_unit_main(void)
{
	//hi2c1.Init.OwnAddress1 = persistence_get_i2c_addr();
	init_modules();
	create_tasks();
}

/**
* @brief  Function implementing the UserInterfaceTa thread.
* @param  argument: Not used
* @retval None
*/
void StartUserInterfaceTask(void *argument)
{
	for(;;)
	{
		interface_loop();
	}
}

/**
* @brief Function implementing the DoorOpsTask thread.
* @param argument: Not used
* @retval None
*/
void StartDoorOpsTask(void *argument)
{
	for(;;)
	{
	  door_control_loop();
	}
}

/**
* @brief Function implementing the CommsTask thread.
* @param argument: Not used
* @retval None
*/
void StartCommsTask(void *argument)
{
	for(;;)
	{
		comms_loop();
	}
}

/**
* @brief Function implementing the DoorSensorTask thread.
* @param argument: Not used
* @retval None
*/
void StartDoorSensorTask(void *argument)
{
  for(;;)
  {
	  door_sensor_loop();
  }
}

/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
void StartDisplayTask(void *argument)
{
	for(;;)
	{
		display_loop();
	}
}
