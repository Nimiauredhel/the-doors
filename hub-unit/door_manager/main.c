#include "hub_common.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "packet_defs.h"
#include "packet_utils.h"
#include "i2c_register_defs.h"

#define TARGET_ADDR_OFFSET 0x30
#define TARGET_ADDR_MAX_COUNT 0x0F

static const char *device_path = "/dev/bone/i2c/2";
static const uint8_t polling_interval_sec = 1;

static int device_fd = 0;

static bool target_addr_set[TARGET_ADDR_MAX_COUNT] = {false};
static uint8_t target_addr_list[TARGET_ADDR_MAX_COUNT] = {0};

static uint8_t target_addr_last_count = 0;

static uint8_t rx_buff[255] = {0};
static DoorPacket_t packet_buff = {0};

static void i2c_init(void)
{
	char buff[64] = {0};

	sprintf(buff, "Opening I2C device at path %s", device_path);
	syslog_append(buff);

	// open the i2c device file
	device_fd = open(device_path, O_RDWR, S_IWUSR);

	if (device_fd <= 0)
	{
		sprintf(buff, "Error opening I2C device, code: %d", device_fd);
		syslog_append(buff);
		exit(EXIT_FAILURE);
	}
	else
	{
		sprintf(buff, "Successfully opened I2C device, descriptor: %d", device_fd);
		syslog_append(buff);
	}
}

static void i2c_set_target(uint8_t addr)
{
	// assign the slave device address
	ioctl(device_fd, I2C_SLAVE, addr);
}

static void i2c_master_write(const uint8_t reg_addr, const uint8_t *message, const uint8_t len)
{
	int32_t result = i2c_smbus_write_i2c_block_data(device_fd, reg_addr, len, message);
	if (result != 0) perror("I2C send error");
	//else printf("Sent %ld bytes.\n", len);
}

static int32_t i2c_master_read(const uint8_t reg_addr, const uint8_t len)
{
	int32_t bytes_read = i2c_smbus_read_i2c_block_data(device_fd, reg_addr, len, rx_buff);
	//printf("Read %ld bytes.\n", bytes_read);
    return bytes_read;
}

static void send_request(DoorRequest_t request, uint32_t extra_data, uint16_t priority)
{
	DoorPacket_t request_buffer;
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

static void poll_slave_event_queue(void)
{
	static const uint8_t zero = 0;

	char debug_buff[32] = {0};
	char syslog_buff[128] = {0};

	syslog_append("Starting polling");

        for(;;)
	{

        sleep(polling_interval_sec);

        for (int i = 0; i < target_addr_last_count; i++)
        {
            i2c_set_target(target_addr_list[i]);

            bzero(rx_buff, sizeof(rx_buff));

            // read event queue length
            // TODO: make this a uint16_t value?
            int32_t bytes_read = i2c_master_read(I2C_REG_EVENT_COUNT, 1);
            uint8_t queue_length = rx_buff[0];
	    //sprintf(syslog_buff, "Read %ld bytes from address [0x%X] event count %u", bytes_read, target_addr_list[i], queue_length);
	    //syslog_append(syslog_buff);


            if (bytes_read <= 0 || queue_length <= 0) continue;
        //		printf("Reported %u new events in slave queue! Reading...\n", queue_length);

            for (int i = 0; i < queue_length; i++)
            {
                bzero(rx_buff, sizeof(rx_buff));
                i2c_master_read(I2C_REG_EVENT_HEAD | (i << 1), sizeof(DoorPacket_t));

                memcpy(&packet_buff, rx_buff, sizeof(DoorPacket_t));

                /*
                printf("[%02u/%02u/%02u][%02u:%02u:%02u]: ",
                        packet_decode_day(packet_buff.header.date),
                        packet_decode_month(packet_buff.header.date),
                        packet_decode_year(packet_buff.header.date),
                        packet_decode_hour(packet_buff.header.time),
                        packet_decode_minutes(packet_buff.header.time),
                        packet_decode_seconds(packet_buff.header.time));
                        */

                DoorReport_t report_type = packet_buff.body.Report.report_id;

                switch(report_type)
                {
                case PACKET_REPORT_NONE:
                     sprintf(syslog_buff, "Door event NONE.");
                     break;
                case PACKET_REPORT_DOOR_OPENED:
                     sprintf(syslog_buff, "Door Opened");
                     break;
                case PACKET_REPORT_DOOR_CLOSED:
                     sprintf(syslog_buff, "Door Closed");
                     break;
                case PACKET_REPORT_DOOR_BLOCKED:
                    sprintf(syslog_buff, "Door Blocked for %lu seconds",
                    packet_buff.body.Report.report_data_32);
                    break;
                case PACKET_REPORT_PASS_CORRECT:
                    bzero(debug_buff, sizeof(debug_buff));
                    door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
                    sprintf(syslog_buff, "Correct Password Entered: %s.", debug_buff);
                    break;
                case PACKET_REPORT_PASS_WRONG:
                    bzero(debug_buff, sizeof(debug_buff));
                    door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
                    sprintf(syslog_buff, "Wrong Password Entered: %s.", debug_buff);
                    break;
                case PACKET_REPORT_PASS_CHANGED:
                    bzero(debug_buff, sizeof(debug_buff));
                    door_pw_to_str(packet_buff.body.Report.report_data_16, debug_buff);
                    debug_buff[4] = '-';
                    debug_buff[5] = '>';
                    door_pw_to_str(packet_buff.body.Report.report_data_32, debug_buff+6);
                    sprintf(syslog_buff, "Password Changed. %s", debug_buff);
                    break;
                case PACKET_REPORT_QUERY_RESULT:
                    sprintf(syslog_buff, "Query Result [%u][%u]", packet_buff.body.Report.report_data_16, packet_buff.body.Report.report_data_32);
                    break;
                case PACKET_REPORT_DATA_READY:
                    sprintf(syslog_buff, "Data Ready.");
                    break;
                case PACKET_REPORT_ERROR:
                    sprintf(syslog_buff, "Error Code [%u][%u]", packet_buff.body.Report.report_data_16, packet_buff.body.Report.report_data_32);
                    break;
                case PACKET_REPORT_TIME_SET:
                    sprintf(syslog_buff, "Time/Date Set to [%u|%u]: %02u/%02u/%02u %02u:%02u:%02u.",
                    packet_buff.body.Report.report_data_16,
                    packet_buff.body.Report.report_data_32,
                    packet_decode_day(packet_buff.body.Report.report_data_16),
                    packet_decode_month(packet_buff.body.Report.report_data_16),
                    packet_decode_year(packet_buff.body.Report.report_data_16),
                    packet_decode_hour(packet_buff.body.Report.report_data_32),
                    packet_decode_minutes(packet_buff.body.Report.report_data_32),
                    packet_decode_seconds(packet_buff.body.Report.report_data_32));
                    break;
                case PACKET_REPORT_FRESH_BOOT:
                    sprintf(syslog_buff, "Fresh Boot. Sending time sync!");
                    send_request(PACKET_REQUEST_SYNC_TIME, 0, 1);
                    break;
                default:
                    sprintf(syslog_buff, "Unknown Door Event");
                    break;
                }

                syslog_append(syslog_buff);
            }

        //	printf("Events retrieved. Ordering slave device to reset its local queue...\n");
            i2c_master_write(I2C_REG_EVENT_COUNT, &zero, 1);
            //printf("Report concluded, polling resumed.\n");
        }
    }
}

static void scan_i2c_bus(void)
{
    char syslog_buff[32] = {0};
    int32_t read_bytes = 0;

    target_addr_last_count = 0;
    explicit_bzero(target_addr_set, TARGET_ADDR_MAX_COUNT);
    explicit_bzero(target_addr_list, TARGET_ADDR_MAX_COUNT);

    sprintf(syslog_buff, "Scanning I2C bus for target devices.");
    syslog_append(syslog_buff);

    for (uint8_t i = 0; i < TARGET_ADDR_MAX_COUNT; i++)
    {
        uint8_t addr = i+TARGET_ADDR_OFFSET;
        i2c_set_target(addr);
        read_bytes = i2c_master_read(I2C_REG_EVENT_COUNT, 1);

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
	    sprintf(syslog_buff, "Concluded I2C bus scan, no devices detected.");
	    syslog_append(syslog_buff);
	    exit(EXIT_FAILURE);
    }

    sprintf(syslog_buff, "Concluded I2C bus scan, %d devices detected.", target_addr_last_count);
    syslog_append(syslog_buff);
}

int main(void)
{
    char buff[64] = {0};

    syslog_init("Hub Door Manager");
    initialize_signal_handler();
    sprintf(buff, "Starting Door Manager, PID %u", getpid());
    syslog_append(buff);
	sleep(1);
	i2c_init();
	sleep(1);
    scan_i2c_bus();
	sleep(1);
	poll_slave_event_queue();
	close(device_fd);
}
