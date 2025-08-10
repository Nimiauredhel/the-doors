/*
 * hub_comms.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "hub_comms.h"

#define GENERAL_COMMAND_QUEUE_CAPACITY 24
#define PRIORITY_COMMAND_QUEUE_CAPACITY 8
#define COMMS_DEBUG_LOG_CAPACITY 32

#define COMMS_LOOP_DELAY_MS (50)
#define COMMS_SEND_INFO_INTERVAL_MS (5000)

#define TAKE_COMMAND_QUEUE_MUTEX \
	if (is_isr()) xSemaphoreTakeFromISR(command_queues_lock, 0); \
	else xSemaphoreTake(command_queues_lock, portMAX_DELAY)
#define GIVE_COMMAND_QUEUE_MUTEX \
	if (is_isr()) xSemaphoreGiveFromISR(command_queues_lock, 0); \
	else xSemaphoreGive(command_queues_lock)

static bool comms_debug_enabled = false;
static CommsEvent_t comms_debug_log[COMMS_DEBUG_LOG_CAPACITY];
static uint16_t comms_debug_pending_count = 0;

static uint16_t outbox_event_count = 0;

static SemaphoreHandle_t command_queues_lock = NULL;
static StaticSemaphore_t command_queues_lock_buffer;
static uint8_t general_command_queue_head = 0;
static uint8_t priority_command_queue_head = 0;
static uint8_t general_command_queue_len = 0;
static uint8_t priority_command_queue_len = 0;
static DoorPacket_t general_command_queue[GENERAL_COMMAND_QUEUE_CAPACITY] = {0};
static DoorPacket_t priority_command_queue[PRIORITY_COMMAND_QUEUE_CAPACITY] = {0};

static void randomize_i2c_address(void)
{
	char buff[32] = {0};
	while(HAL_OK != HAL_I2C_DisableListen_IT(&hi2c1));
	while(HAL_OK != HAL_I2C_DeInit(&hi2c1));
	int new_address = random_range(I2C_MIN_ADDRESS, I2C_MAX_ADDRESS);
	sprintf(buff, "Randomized I2C address: [0x%X]", new_address);
	serial_print_line(buff, 0);
	hi2c1.Init.OwnAddress1 = new_address << 1;
	while(HAL_OK != HAL_I2C_Init(&hi2c1));
	while(HAL_OK != HAL_I2C_EnableListen_IT(&hi2c1));
}

static void comms_debug_output(void)
{
	char buff[64] = {0};
	uint8_t count = 0;

	if (comms_debug_enabled)
	{
		uint16_t outbox_current_length = event_log_get_length();

		if (outbox_event_count < outbox_current_length)
		{
			outbox_event_count = outbox_current_length;
			sprintf(buff, "Event Log Outbox has %u events.", outbox_event_count);
			serial_print_line(buff, 0);
		}
		else if (outbox_event_count > 0 && outbox_current_length == 0)
		{
			outbox_event_count = 0;
			serial_print_line("Log has been cleared!", 0);
		}
	}

	if (comms_debug_pending_count > 0)
	{
		count = comms_debug_pending_count;
		comms_debug_pending_count = 0;

		if (comms_debug_enabled)
		{
			for (int i = 0; i < count; i++)
			{
				sprintf(buff, "I2C reports: %s event on %s.",
						comms_debug_log[i].action == COMMS_EVENT_SENT ? "TX" : "RX",
						i2c_register_names[comms_debug_log[i].subject/2]);
				serial_print_line(buff, 0);
			}
		}
	}
}

static void comms_process_command(DoorPacket_t *cmd_ptr)
{
	switch (cmd_ptr->body.Request.request_id)
	{
	case PACKET_REQUEST_NONE:
		serial_print_line("Received NULL command from hub.", 0);
		break;
	case PACKET_REQUEST_DOOR_CLOSE:
		serial_print_line("Received CLOSE DOOR command from hub.", 0);
		door_set_closed(true);
		break;
	case PACKET_REQUEST_DOOR_OPEN:
		serial_print_line("Received OPEN DOOR command from hub.", 0);
		door_set_closed(false);
		break;
	case PACKET_REQUEST_BELL:
		serial_print_line("\aReceived bell command, this is unusual.", 0);
		break;
	case PACKET_REQUEST_PHOTO:
		// TODO: implement the photo request
		serial_print_line("Received photo request, yet unimplemented.", 0);
		break;
	case PACKET_REQUEST_SYNC_TIME:
		serial_print_line("Received SYNC TIME command from hub.", 0);
		date_time_set_from_packet(cmd_ptr->header.date, cmd_ptr->header.time);
		break;
	case PACKET_REQUEST_SYNC_PASS_USER:
		serial_print_line("Received SYNC PASS USER command from hub, setting password.", 0);
		auth_set_user_pass_internal(cmd_ptr->body.Request.request_data_32);
		break;
	case PACKET_REQUEST_SYNC_PASS_ADMIN:
		serial_print_line("Received SYNC PASS ADMIN command from hub, setting password.", 0);
		auth_set_admin_pass_internal(cmd_ptr->body.Request.request_data_32);
		break;
	case PACKET_REQUEST_RESET_ADDRESS:
		serial_print_line("Received RESET ADDRESS command from hub, randomizing new address.", 0);
		randomize_i2c_address();
		break;
	case PACKET_REQUEST_PING:
		serial_print_line("Received PING from hub.", 0);
		event_log_append_report_minimal(PACKET_REPORT_PONG);
		break;
	case PACKET_REQUEST_MAX:
		serial_print_line("Received MAX command from hub, this makes no sense.", 0);
		break;
	}
}

static void comms_check_command_queue(void)
{
	uint8_t cmd_idx = 0;
	DoorPacket_t *cmd_ptr = NULL;

	TAKE_COMMAND_QUEUE_MUTEX;

	// process entire priority queue before proceeding
	while (priority_command_queue_len > 0)
	{
		cmd_idx = priority_command_queue_head + (priority_command_queue_len - 1);
		if (cmd_idx > PRIORITY_COMMAND_QUEUE_CAPACITY) cmd_idx -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		cmd_ptr = priority_command_queue+cmd_idx;
		priority_command_queue_head++;
		if (priority_command_queue_head > PRIORITY_COMMAND_QUEUE_CAPACITY) priority_command_queue_head -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		priority_command_queue_len--;
		comms_process_command(cmd_ptr);
	}

	// process one item at a time from the general queue to avoid blocking
	if (general_command_queue_len > 0)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		cmd_idx = general_command_queue_head + (general_command_queue_len - 1);
		if (cmd_idx > GENERAL_COMMAND_QUEUE_CAPACITY) cmd_idx -= GENERAL_COMMAND_QUEUE_CAPACITY;
		cmd_ptr = general_command_queue+cmd_idx;
		general_command_queue_head++; if (general_command_queue_head > GENERAL_COMMAND_QUEUE_CAPACITY) general_command_queue_head -= GENERAL_COMMAND_QUEUE_CAPACITY;
		general_command_queue_len--;
		comms_process_command(cmd_ptr);
	}

	GIVE_COMMAND_QUEUE_MUTEX;
}

void comms_init(void)
{
	command_queues_lock = xSemaphoreCreateMutexStatic(&command_queues_lock_buffer);
	//randomize_i2c_address();
	i2c_io_init();
	serial_print_line("Initialized I2C listening to hub unit.", 0);
	comms_send_info();
}

void comms_loop(void)
{
	static uint16_t send_info_counter_ms = 0;

	vTaskDelay(pdMS_TO_TICKS(COMMS_LOOP_DELAY_MS));
	send_info_counter_ms += COMMS_LOOP_DELAY_MS;

	comms_check_command_queue();
	comms_debug_output();

	if (send_info_counter_ms >= COMMS_SEND_INFO_INTERVAL_MS)
	{
		send_info_counter_ms = 0;
		comms_send_info();
	}
}

void comms_enqueue_command(DoorPacket_t *cmd_ptr)
{
	uint8_t tail_idx;

	TAKE_COMMAND_QUEUE_MUTEX;

	// TODO: this code is begging for a reusable packet queue implementation
	if (cmd_ptr->header.priority > 0)
	{
		if (priority_command_queue_len >= PRIORITY_COMMAND_QUEUE_CAPACITY) return;
		tail_idx = priority_command_queue_head + priority_command_queue_len;
		if (tail_idx >= PRIORITY_COMMAND_QUEUE_CAPACITY) tail_idx -= PRIORITY_COMMAND_QUEUE_CAPACITY;
		memcpy(priority_command_queue+tail_idx, cmd_ptr, sizeof(DoorPacket_t));
		priority_command_queue_len++;
	}
	else
	{
		if (general_command_queue_len >= GENERAL_COMMAND_QUEUE_CAPACITY) return;
		tail_idx = general_command_queue_head + general_command_queue_len;
		if (tail_idx >= GENERAL_COMMAND_QUEUE_CAPACITY) tail_idx -= GENERAL_COMMAND_QUEUE_CAPACITY;
		memcpy(general_command_queue+tail_idx, cmd_ptr, sizeof(DoorPacket_t));
		general_command_queue_len++;
	}

	GIVE_COMMAND_QUEUE_MUTEX;
}

void comms_report_internal(CommsEventType_t action, I2CRegisterDefinition_t subject)
{
	if (!comms_debug_enabled) return;
	// TODO: maybe use a mutex here
	if (comms_debug_pending_count > COMMS_DEBUG_LOG_CAPACITY) return;
	comms_debug_log[comms_debug_pending_count].action = action;
	comms_debug_log[comms_debug_pending_count].subject = subject;
	comms_debug_pending_count++;
}

void comms_toggle_debug(void)
{
	comms_debug_enabled = !comms_debug_enabled;
}

void comms_send_info(void)
{
	uint8_t packet_buff[sizeof(DoorPacket_t) + sizeof(DoorInfo_t)] = {0};
	DoorPacket_t *packet_ptr = (DoorPacket_t *)packet_buff;
	DoorInfo_t *info_ptr = (DoorInfo_t *)(packet_buff + sizeof(DoorPacket_t));

	packet_ptr->header.category = PACKET_CAT_DATA;
	packet_ptr->body.Data.data_type = PACKET_DATA_DOOR_INFO;
	packet_ptr->body.Data.source_id = i2c_io_get_device_id();
	packet_ptr->body.Data.data_length = sizeof(DoorInfo_t);

	persistence_get_name(info_ptr->name);
	info_ptr->index = 12;
	info_ptr->i2c_address = persistence_get_i2c_addr() >> 1;

	i2c_send_data(PACKET_DATA_DOOR_INFO, packet_buff, sizeof(packet_buff));
}
