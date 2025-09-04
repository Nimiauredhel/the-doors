#include "door_manager_i2c.h"

static const char device_path[16] = "/dev/bone/i2c/2";
static const uint8_t polling_interval_sec = 1;

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

static int32_t i2c_master_read(const uint8_t reg_addr, const uint8_t len, uint8_t *dst)
{
    int32_t bytes_read = i2c_smbus_read_i2c_block_data(device_fd, reg_addr, len, dst);
    return bytes_read;
}

static void read_data_from_door(uint16_t length)
{
    static const uint8_t chunk_len = 24;

    static char syslog_buff[64] = {0};

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

    snprintf(syslog_buff, sizeof(syslog_buff), "Read %d data bytes out of target %u.", total_bytes_read, length);
    syslog_append(syslog_buff);
}

static void process_data_from_door(void)
{
    char syslog_buff[128] = {0};

    DoorDataType_t data_type = rx_packet_ptr->body.Data.data_type;
    DoorInfo_t *door_info_ptr = (DoorInfo_t *)rx_data_ptr;

    switch(data_type)
    {
    case PACKET_DATA_DOOR_INFO:
        snprintf(syslog_buff, sizeof(syslog_buff), "Received door info, name: %s, address: %u, index: %u, from actual address: %u, expected index: %u.",
                door_info_ptr->name, INDEX_TO_I2C_ADDR(door_info_ptr->index), door_info_ptr->index, current_target_addr, I2C_ADDR_TO_INDEX(current_target_addr));
        syslog_append(syslog_buff);

        if (door_info_ptr->index >= HUB_MAX_DOOR_COUNT)
        {
            sprintf(syslog_buff, "Received index out of bounds.");
            syslog_append(syslog_buff);
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
                    sprintf(syslog_buff, "Updating existing entry with received data.");
                    syslog_append(syslog_buff);
                    break;
                }
            }

            if (cell < 0)
            {
                sprintf(syslog_buff, "Door ID unused, storing new entry.");
                syslog_append(syslog_buff);

                cell = door_states_ptr->count;
                door_states_ptr->count++;
                door_states_ptr->id[cell] = door_info_ptr->index;
            }

            door_states_ptr->last_seen[cell] = time(NULL);
            strncpy(door_states_ptr->name[cell], door_info_ptr->name, UNIT_NAME_MAX_LEN);

            FILE *file = fopen("site/doors.txt", "w");

            for (int i = 0; i < door_states_ptr->count; i++)
            {
                fprintf(file, " [%u] %s [%ld]\n", door_states_ptr->id[i], door_states_ptr->name[i], door_states_ptr->last_seen[i]);
            }

            fclose(file);

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
	char syslog_buff[128] = {0};

    DoorReport_t report_type = rx_packet_ptr->body.Report.report_id;

    switch(report_type)
    {
    case PACKET_REPORT_NONE:
         sprintf(syslog_buff, "Door event NONE.");
         syslog_append(syslog_buff);
         break;
    case PACKET_REPORT_DOOR_OPENED:
         sprintf(syslog_buff, "Door Opened");
         syslog_append(syslog_buff);
         break;
    case PACKET_REPORT_DOOR_CLOSED:
         sprintf(syslog_buff, "Door Closed");
         syslog_append(syslog_buff);
         break;
    case PACKET_REPORT_DOOR_BLOCKED:
        sprintf(syslog_buff, "Door Blocked for %u seconds", rx_packet_ptr->body.Report.report_data_32);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_PASS_CORRECT:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        sprintf(syslog_buff, "Correct Password Entered: %s.", debug_buff);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_PASS_WRONG:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        sprintf(syslog_buff, "Wrong Password Entered: %s.", debug_buff);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_PASS_CHANGED:
        bzero(debug_buff, sizeof(debug_buff));
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_16, debug_buff);
        debug_buff[4] = '-';
        debug_buff[5] = '>';
        door_pw_to_str(rx_packet_ptr->body.Report.report_data_32, debug_buff+6);
        sprintf(syslog_buff, "Password Changed. %s", debug_buff);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_QUERY_RESULT:
        sprintf(syslog_buff, "Query Result [%u][%u]", rx_packet_ptr->body.Report.report_data_16, rx_packet_ptr->body.Report.report_data_32);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_DATA_READY:
        snprintf(syslog_buff, sizeof(syslog_buff), "Data Ready from source [%u], data size [%u].",
                rx_packet_ptr->body.Report.source_id, rx_packet_ptr->body.Report.report_data_32);
        syslog_append(syslog_buff);
        read_data_from_door(rx_packet_ptr->body.Report.report_data_32);
        process_data_from_door();
        break;
    case PACKET_REPORT_ERROR:
        sprintf(syslog_buff, "Error Code [%u][%u]", rx_packet_ptr->body.Report.report_data_16, rx_packet_ptr->body.Report.report_data_32);
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_TIME_SET:
        sprintf(syslog_buff, "Time/Date Set to [%u|%u]: %02u/%02u/%02u %02u:%02u:%02u.",
        rx_packet_ptr->body.Report.report_data_16,
        rx_packet_ptr->body.Report.report_data_32,
        packet_decode_day(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_month(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_year(rx_packet_ptr->body.Report.report_data_16),
        packet_decode_hour(rx_packet_ptr->body.Report.report_data_32),
        packet_decode_minutes(rx_packet_ptr->body.Report.report_data_32),
        packet_decode_seconds(rx_packet_ptr->body.Report.report_data_32));
        syslog_append(syslog_buff);
        break;
    case PACKET_REPORT_FRESH_BOOT:
        sprintf(syslog_buff, "Fresh Boot. Sending time sync!");
        syslog_append(syslog_buff);
        send_request_to_door(PACKET_REQUEST_SYNC_TIME, 0, 1);
        break;
    default:
        sprintf(syslog_buff, "Unknown Door Event");
        syslog_append(syslog_buff);
        break;
    }
}

static void process_request_from_door(void)
{
	char syslog_buff[128] = {0};

    DoorRequest_t request_type = rx_packet_ptr->body.Request.request_id;

    switch(request_type)
    {
    case PACKET_REQUEST_BELL:
        sprintf(syslog_buff, "Forwarding bell from door %u to client %u.", rx_packet_ptr->body.Request.source_id, rx_packet_ptr->body.Request.destination_id);
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

    syslog_append(syslog_buff);
}

static void poll_slave_event_queue(void)
{
	static const uint8_t zero = 0;

	char syslog_buff[128] = {0};

	//syslog_append("Starting polling");

    sleep(polling_interval_sec);

    for (int i = 0; i < target_addr_last_count; i++)
    {
        i2c_set_target(target_addr_list[i]);

        bzero(rx_buff, sizeof(rx_buff));

        // read event queue length
        // TODO: make this a uint16_t value?
        int32_t bytes_read = i2c_master_read(I2C_REG_EVENT_COUNT, 1, rx_buff);
        uint8_t queue_length = rx_buff[0];
    //sprintf(syslog_buff, "Read %ld bytes from address [0x%X] event count %u", bytes_read, target_addr_list[i], queue_length);
    //syslog_append(syslog_buff);


        if (bytes_read <= 0 || queue_length <= 0) continue;
    //		printf("Reported %u new events in slave queue! Reading...\n", queue_length);

        for (int j = 0; j < queue_length; j++)
        {
            bzero(rx_buff, sizeof(rx_buff));
            i2c_master_read(I2C_REG_EVENT_HEAD | (j << 1), sizeof(DoorPacket_t), rx_buff);

            /*
            printf("[%02u/%02u/%02u][%02u:%02u:%02u]: ",
                    packet_decode_day(packet_ptr->header.date),
                    packet_decode_month(packet_ptr->header.date),
                    packet_decode_year(packet_ptr->header.date),
                    packet_decode_hour(packet_ptr->header.time),
                    packet_decode_minutes(packet_ptr->header.time),
                    packet_decode_seconds(packet_ptr->header.time));
                    */


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

        //printf("Events retrieved. Ordering slave device to reset its local queue...\n");
        i2c_master_write(I2C_REG_EVENT_COUNT, &zero, 1);
        //printf("Report concluded, polling resumed.\n");
    }
}

static void scan_i2c_bus(void)
{
    char syslog_buff[128] = {0};
    int32_t read_bytes = 0;

    explicit_bzero(target_addr_set, TARGET_ADDR_MAX_COUNT);
    explicit_bzero(target_addr_list, TARGET_ADDR_MAX_COUNT);

    target_addr_last_count = 0;

    while (target_addr_last_count == 0)
    {
        sprintf(syslog_buff, "Scanning I2C bus for target devices.");
        syslog_append(syslog_buff);

        for (uint8_t i = 0; i < TARGET_ADDR_MAX_COUNT; i++)
        {
            uint8_t addr = i+TARGET_ADDR_OFFSET;
            i2c_set_target(addr);
            read_bytes = i2c_master_read(I2C_REG_EVENT_COUNT, 1, rx_buff);

        //    sprintf(syslog_buff, "Addr [0x%X], read bytes: %d", addr, read_bytes);
        //    syslog_append(syslog_buff);

            if (read_bytes > 0)
            {
                target_addr_set[i] = true;
                target_addr_list[target_addr_last_count] = addr;
                sprintf(syslog_buff, "Detected target device at address [0x%X].", target_addr_list[target_addr_last_count]);
                syslog_append(syslog_buff);
                target_addr_last_count += 1;
            }
        }

        if (target_addr_last_count == 0)
        {
            snprintf(syslog_buff, sizeof(syslog_buff), "Concluded I2C bus scan, no devices detected - retrying in 2 seconds.");
            syslog_append(syslog_buff);
            sleep(2);
        }
    }

    sprintf(syslog_buff, "Concluded I2C bus scan, %d devices detected.", target_addr_last_count);
    syslog_append(syslog_buff);
}

void i2c_forward_request(DoorPacket_t *request)
{
    syslog_append("Forwarding request to door");
    if (target_addr_last_count > 0)
    {
        i2c_set_target(target_addr_list[0]);
    }
    i2c_master_write(I2C_REG_HUB_COMMAND, (const uint8_t *)request, sizeof(DoorPacket_t));
}

static void i2c_loop(void)
{
    static const uint16_t rescan_threshold = 100;
    static uint16_t rescan_counter = 0;

    if (rescan_counter <= 0)
    {
        rescan_counter = rescan_threshold;
        scan_i2c_bus();
    }
    else
    {
        rescan_counter--;
    }

    poll_slave_event_queue();
}

void i2c_terminate(void)
{
    if (device_fd > 0)
    {
        close(device_fd);
    }
}

void* i2c_task(void *arg)
{
    for(;;) i2c_loop();
    return NULL;
}

void i2c_init(void)
{
	char buff[64] = {0};

	system("sh i2c2_init.sh");

	sprintf(buff, "Opening I2C device at path %s", device_path);
	syslog_append(buff);

	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);

	if (device_fd <= 0)
	{
		sprintf(buff, "Error opening I2C device, code: %d", device_fd);
		syslog_append(buff);
        common_terminate(EXIT_FAILURE);
	}
	else
	{
		sprintf(buff, "Successfully opened I2C device, descriptor: %d", device_fd);
		syslog_append(buff);
	}
}
