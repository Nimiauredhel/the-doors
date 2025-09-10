#include "door_manager_i2c.h"

static const char device_path[16] = "/dev/bone/i2c/2";
static const struct timespec polling_loop_delay =
{
    .tv_nsec = 500000000,
    .tv_sec = 0,
};
static const struct timespec scanning_loop_delay =
{
    .tv_nsec = 0,
    .tv_sec = 2,
};

static int device_fd = -1;

static bool target_addr_set[TARGET_ADDR_MAX_COUNT] = {false};
static uint8_t target_addr_list[TARGET_ADDR_MAX_COUNT] = {0};
static uint8_t current_target_addr = 0;

static uint8_t target_addr_last_count = 0;

static uint8_t rx_buff[sizeof(DoorPacket_t) + DOOR_DATA_BYTES_LARGE] = {0};
static DoorPacket_t *rx_packet_ptr = (DoorPacket_t *)&rx_buff;
static void *rx_data_ptr = rx_buff + sizeof(DoorPacket_t);

static void i2c_set_target(uint8_t addr)
{
	// assign the slave device address
    current_target_addr = addr;
	ioctl(device_fd, I2C_SLAVE, addr);
}

static void i2c_master_write(const uint8_t reg_addr, const uint8_t *message, const uint8_t len)
{
	int32_t result = i2c_smbus_write_i2c_block_data(device_fd, reg_addr, len, message);
	if (result != 0) perror("I2C send error");
	//else printf("Sent %ld bytes.\n", len);
}

static void send_request_to_door(DoorRequest_t request, uint32_t extra_data, uint16_t priority)
{
	DoorPacket_t request_buffer = {0};
	struct tm dt = get_datetime();

	request_buffer.header.category = PACKET_CAT_REQUEST;
	request_buffer.header.priority = priority;
	request_buffer.header.date =
		packet_encode_date(dt.tm_year-100, dt.tm_mon+1, dt.tm_mday);
	request_buffer.header.time =
		packet_encode_time(dt.tm_hour, dt.tm_min, dt.tm_sec);

	request_buffer.body.Request.request_id = request;
	request_buffer.body.Request.request_data_32 = extra_data;

	i2c_master_write(I2C_REG_HUB_COMMAND, (const uint8_t *)&request_buffer, sizeof(DoorPacket_t));
}

/**
 * @brief Attempts to read <length> bytes from the currently targeted Door unit's <reg_addr> register, to buffer <dst>.
 * @param [in] reg_addr The target register to read from.
 * @param [in] length The number of bytes to be read.
 * @param [out] dst The destination buffer to which the bytes will be written.
 **/
static int32_t i2c_master_read(const uint8_t reg_addr, const uint8_t length, uint8_t *dst)
{
    int32_t bytes_read = i2c_smbus_read_i2c_block_data(device_fd, reg_addr, length, dst);
    return bytes_read;
}

/**
 * @brief Attempts to read <length> bytes from the currently targeted Door unit's Data register.
 * @param [in] length The number of bytes to be read.
 **/
static void read_data_from_door(uint16_t length)
{
    static const uint8_t chunk_len = 24;

    static char log_buff[128] = {0};

    uint16_t offset = 0;
    uint16_t remaining = length;

    int32_t total_bytes_read = 0;

    while(remaining > 0)
    {
        uint8_t to_read = remaining > chunk_len ? chunk_len : remaining;

        int32_t bytes_read = i2c_master_read(I2C_REG_DATA, to_read, rx_buff+offset);

        if (bytes_read < 0) break;
        remaining -= bytes_read;
        total_bytes_read += bytes_read;
        if (bytes_read < chunk_len) break;

        offset += bytes_read;
    }

    snprintf(log_buff, sizeof(log_buff), "Read %d data bytes out of target %u.", total_bytes_read, length);
    log_append(log_buff);
}

static void process_data_from_door(void)
{
    char log_buff[128] = {0};

    DoorDataType_t data_type = rx_packet_ptr->body.Data.data_type;
    DoorInfo_t *door_info_ptr = (DoorInfo_t *)rx_data_ptr;

    switch(data_type)
    {
    case PACKET_DATA_DOOR_INFO:
        snprintf(log_buff, sizeof(log_buff), "Received door info, name: %s, address: %u, index: %u, from actual address: %u, expected index: %u.",
                door_info_ptr->name, INDEX_TO_I2C_ADDR(door_info_ptr->index), door_info_ptr->index, current_target_addr, I2C_ADDR_TO_INDEX(current_target_addr));
        log_append(log_buff);

        if (door_info_ptr->index >= HUB_MAX_DOOR_COUNT)
        {
            log_append("Received index out of bounds.");
        }
        else
        {
            HubDoorStates_t *door_states_ptr = ipc_acquire_door_states_ptr();

            int8_t cell = -1;

            for (int i = 0; i < door_states_ptr->count; i++)
            {
                if (door_states_ptr->id[i] == door_info_ptr->index)
                {
                    cell = i;
                    log_append("Updating existing entry with received data.");
                    break;
                }
            }

            if (cell < 0)
            {
                log_append("Door ID unused, storing new entry.");

                cell = door_states_ptr->count;
                door_states_ptr->count++;
                door_states_ptr->id[cell] = door_info_ptr->index;
            }

            door_states_ptr->last_seen[cell] = time(NULL);
            strncpy(door_states_ptr->name[cell], door_info_ptr->name, UNIT_NAME_MAX_LEN);

            door_manager_update_door_list_txt(door_states_ptr);

            ipc_release_door_states_ptr();
        }
	break;
    case PACKET_DATA_NONE:
    case PACKET_DATA_CLIENT_INFO:
    case PACKET_DATA_IMAGE:
    case PACKET_DATA_MAX:
    default:
        break;
    }
}

static void process_report(void)
{
	char debug_buff[32] = {0};
	char log_buff[128] = {0};

    DoorReport_t report_type = rx_packet_ptr->body.Report.report_id;

    switch(report_type)
    {
    case PACKET_REPORT_NONE:
         snprintf(log_buff, sizeof(log_buff), "Door %u event: NONE.", rx_packet_ptr->body.Report.source_id);
         log_append(log_buff);
         break;
    case PACKET_REPORT_DOOR_OPENED:
         snprintf(log_buff, sizeof(log_buff), "Door %u: Opened", rx_packet_ptr->body.Report.source_id);
         log_append(log_buff);
         break;
    case PACKET_REPORT_DOOR_CLOSED:
         snprintf(log_buff, sizeof(log_buff), "Door %u: Closed", rx_packet_ptr->body.Report.source_id);
         log_append(log_buff);
         break;
    case PACKET_REPORT_DOOR_BLOCKED:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Blocked for %u seconds", rx_packet_ptr->body.Report.source_id, rx_packet_ptr->body.Report.report_data_32);
        log_append(log_buff);
        break;
    case PACKET_REPORT_PASS_CORRECT:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        snprintf(log_buff, sizeof(log_buff), "Door %u: Correct Password Entered: %s.", rx_packet_ptr->body.Report.source_id, debug_buff);
        log_append(log_buff);
        break;
    case PACKET_REPORT_PASS_WRONG:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        snprintf(log_buff, sizeof(log_buff), "Door %u: Wrong Password Entered: %s.", rx_packet_ptr->body.Report.source_id, debug_buff);
        log_append(log_buff);
        break;
    case PACKET_REPORT_PASS_CHANGED:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        debug_buff[4] = '-';
        debug_buff[5] = '>';
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_32, debug_buff+6);
        snprintf(log_buff, sizeof(log_buff), "Door %u: Password Changed. %s", rx_packet_ptr->body.Report.source_id, debug_buff);
        log_append(log_buff);
        break;
    case PACKET_REPORT_QUERY_RESULT:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Query Result [%u][%u]", rx_packet_ptr->body.Report.source_id, rx_packet_ptr->body.Report.report_data_16, rx_packet_ptr->body.Report.report_data_32);
        log_append(log_buff);
        break;
    case PACKET_REPORT_DATA_READY:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Data Ready, size [%u].",
                 rx_packet_ptr->body.Report.source_id, rx_packet_ptr->body.Report.report_data_32);
        log_append(log_buff);
        read_data_from_door(rx_packet_ptr->body.Report.report_data_32);
        process_data_from_door();
        break;
    case PACKET_REPORT_ERROR:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Error Code [%u][%u]", rx_packet_ptr->body.Report.source_id, rx_packet_ptr->body.Report.report_data_16, rx_packet_ptr->body.Report.report_data_32);
        log_append(log_buff);
        break;
    case PACKET_REPORT_TIME_SET:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Time/Date Set to [%u|%u]: %02u/%02u/%02u %02u:%02u:%02u.",
        rx_packet_ptr->body.Report.source_id,
        rx_packet_ptr->body.Report.report_data_16,
        rx_packet_ptr->body.Report.report_data_32,
        packet_decode_day(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_month(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_year(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_hour(rx_packet_ptr->body.Report.report_data_32),
        packet_decode_minutes(rx_packet_ptr->body.Report.report_data_32),
        packet_decode_seconds(rx_packet_ptr->body.Report.report_data_32));
        log_append(log_buff);
        break;
    case PACKET_REPORT_FRESH_BOOT:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Fresh Boot. Sending time sync!", rx_packet_ptr->body.Report.source_id);
        log_append(log_buff);
        send_request_to_door(PACKET_REQUEST_SYNC_TIME, 0, 1);
        break;
    default:
        snprintf(log_buff, sizeof(log_buff), "Door %u: Unknown Event", rx_packet_ptr->body.Report.source_id);
        log_append(log_buff);
        break;
    }
}

static void process_request_from_door(void)
{
	char log_buff[128] = {0};

    DoorRequest_t request_type = rx_packet_ptr->body.Request.request_id;

    switch(request_type)
    {
    case PACKET_REQUEST_BELL:
        snprintf(log_buff, sizeof(log_buff), "Forwarding bell from door %u to client %u.", rx_packet_ptr->body.Request.source_id, rx_packet_ptr->body.Request.destination_id);
        hub_queue_enqueue(doors_to_clients_queue, rx_packet_ptr);
        break;
    case PACKET_REQUEST_SYNC_TIME:
        break;
    case PACKET_REQUEST_SYNC_PASS_USER:
        break;
    case PACKET_REQUEST_SYNC_PASS_ADMIN:
        break;
    case PACKET_REQUEST_PING:
        break;
    case PACKET_REQUEST_NONE:
    case PACKET_REQUEST_DOOR_OPEN:
    case PACKET_REQUEST_DOOR_CLOSE:
    case PACKET_REQUEST_PHOTO:
    case PACKET_REQUEST_RESET_ADDRESS:
    case PACKET_REQUEST_MAX:
    default:
      break;
    }

    log_append(log_buff);
}

static void poll_slave_event_queue(void)
{
	static const uint8_t zero = 0;

    for (int i = 0; i < target_addr_last_count; i++)
    {
        i2c_set_target(target_addr_list[i]);

        bzero(rx_buff, sizeof(rx_buff));

        // read event queue length
        int32_t bytes_read = i2c_master_read(I2C_REG_EVENT_COUNT, 1, rx_buff);
        uint8_t queue_length = rx_buff[0];

        if (bytes_read <= 0 || queue_length <= 0) continue;

        for (int j = 0; j < queue_length; j++)
        {
            bzero(rx_buff, sizeof(rx_buff));
            i2c_master_read(I2C_REG_EVENT_HEAD | (j << 1), sizeof(DoorPacket_t), rx_buff);

            switch(rx_packet_ptr->header.category)
            {
            case PACKET_CAT_REPORT:
                rx_packet_ptr->body.Report.source_id = i;
                process_report();
                break;
            case PACKET_CAT_REQUEST:
                rx_packet_ptr->body.Request.source_id = i;
                process_request_from_door();
                break;
            case PACKET_CAT_DATA:
                // shouldn't happen
                rx_packet_ptr->body.Data.source_id = i;
                process_data_from_door();
                break;
            case PACKET_CAT_NONE:
            case PACKET_CAT_MAX:
            default:
              break;
            }

        }

        i2c_master_write(I2C_REG_EVENT_COUNT, &zero, 1);
    }
}

static void scan_i2c_bus(void)
{
    char log_buff[128] = {0};
    int32_t read_bytes = 0;

    explicit_bzero(target_addr_set, TARGET_ADDR_MAX_COUNT);
    explicit_bzero(target_addr_list, TARGET_ADDR_MAX_COUNT);

    target_addr_last_count = 0;

    while (target_addr_last_count == 0)
    {
        log_append("Scanning I2C bus for target devices.");

        for (uint8_t i = 0; i < TARGET_ADDR_MAX_COUNT; i++)
        {
            uint8_t addr = i+TARGET_ADDR_OFFSET;
            i2c_set_target(addr);
            read_bytes = i2c_master_read(I2C_REG_EVENT_COUNT, 1, rx_buff);

            if (read_bytes > 0)
            {
                target_addr_set[i] = true;
                target_addr_list[target_addr_last_count] = addr;
                snprintf(log_buff, sizeof(log_buff), "Detected target device at address [0x%X].", target_addr_list[target_addr_last_count]);
                log_append(log_buff);
                target_addr_last_count += 1;
            }
        }

        if (target_addr_last_count == 0)
        {
            log_append("Concluded I2C bus scan, no devices detected - retrying in 2 seconds.");
            nanosleep(&scanning_loop_delay, NULL);
        }
    }

    snprintf(log_buff, sizeof(log_buff), "Concluded I2C bus scan, %d devices detected.", target_addr_last_count);
    log_append(log_buff);
}

/**
 * @brief Accepts a 'request' type packet, and forwards it
 * to its intended recipient Door Unit on the I2C bus.
 **/
void i2c_forward_request(DoorPacket_t *request)
{
    log_append("Forwarding request to door");
    if (target_addr_last_count > 0)
    {
        i2c_set_target(target_addr_list[0]);
    }
    i2c_master_write(I2C_REG_HUB_COMMAND, (const uint8_t *)request, sizeof(DoorPacket_t));
}

static void i2c_loop(void)
{
    static const int16_t rescan_threshold_loops = 60;
    static int16_t rescan_counter_loops = 0;

    if (rescan_counter_loops <= 0)
    {
        rescan_counter_loops = rescan_threshold_loops;
        scan_i2c_bus();
    }
    else
    {
        rescan_counter_loops--;
    }

    poll_slave_event_queue();

    nanosleep(&polling_loop_delay, NULL);
}

void i2c_deinit(void)
{
    if (device_fd > 0)
    {
        close(device_fd);
    }
}

void* i2c_task(void *arg)
{
    (void)arg;

    while(!should_terminate) i2c_loop();
    return NULL;
}

void i2c_init(void)
{
	char buff[64] = {0};

	if (0 > system("sh i2c2_init.sh"))
    {
        log_append("Failed to run I2C2 initialization script.");
        should_terminate = true;
        return;
    }

	sprintf(buff, "Opening I2C device at path %s", device_path);
	log_append(buff);

	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);

	if (device_fd <= 0)
	{
		sprintf(buff, "Error opening I2C device, code: %d", device_fd);
		log_append(buff);
        should_terminate = true;
        return;
	}
	else
	{
		sprintf(buff, "Successfully opened I2C device, descriptor: %d", device_fd);
		log_append(buff);
	}
}
