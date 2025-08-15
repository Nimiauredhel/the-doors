#include "client.h"

static int client_socket = -1;
static struct sockaddr_in client_addr = {0};
static struct sockaddr_in server_addr ={0};

static volatile ClientState_t client_state = CLIENTSTATE_NONE;
static volatile ClientState_t client_state_out = CLIENTSTATE_NONE;

static volatile int8_t current_bell_idx = -1;

static ClientDoorList_t door_list = {0};

static uint8_t rx_buff[sizeof(DoorPacket_t) + DOOR_DATA_BYTES_LARGE] = {0};
static DoorPacket_t *rx_packet_ptr = (DoorPacket_t *)rx_buff;
static DoorInfo_t *rx_info_ptr = (DoorInfo_t *)(rx_buff + sizeof(DoorPacket_t));

ClientDoorList_t *client_acquire_door_list_ptr(void)
{
    xSemaphoreTake(door_list.lock, portMAX_DELAY);
    return &door_list;
}

void client_release_door_list_ptr(void)
{
    xSemaphoreGive(door_list.lock);
}

int8_t client_get_current_bell_idx(void)
{
    return current_bell_idx;
}

esp_netif_ip_info_t client_get_ip_info(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_info);
    return ip_info;
}

int client_send_request(DoorRequest_t request, uint16_t destination)
{
    struct tm datetime = get_datetime();

    DoorPacket_t tx_buff =
    {
        // TODO: include source, version, priority etc
        .header.time = packet_encode_time(datetime.tm_hour, datetime.tm_min, datetime.tm_sec),
        .header.date = packet_encode_date(datetime.tm_year, datetime.tm_mon, datetime.tm_mday),
        .header.category = PACKET_CAT_REQUEST,
        .body.Request.destination_id = destination,
        .body.Request.request_id = request,
    };

    return send(client_socket, &tx_buff, sizeof(tx_buff), 0);
}

static struct sockaddr_in init_server_socket_address(struct in_addr peer_address_bin, in_port_t peer_port_bin)
{
    struct sockaddr_in socket_address =
    {
        .sin_family = AF_INET,
        .sin_port = peer_port_bin,
        .sin_addr = peer_address_bin
    };

    return socket_address;
}

static void init_local_data_socket(int *socket_ptr, struct sockaddr_in *address_ptr)
{
    //static const int reuse_flag = 1;
    static const struct timeval socket_timeout = { .tv_sec = 1, .tv_usec = 0 };

    *socket_ptr = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (*socket_ptr < 0)
    {
        perror("Failed to create data socket");
        printf("Restarting in 2 seconds...\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }

    /*
    if(0 > setsockopt(*socket_ptr, SOL_SOCKET, SO_REUSEADDR,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse address' option");
        exit(EXIT_FAILURE);
    }

    if(0 > setsockopt(*socket_ptr, SOL_SOCKET, SO_REUSEPORT,  &reuse_flag, sizeof(reuse_flag)))
    {
        perror("Failed to set socket 'reuse port' option");
        exit(EXIT_FAILURE);
    }
    */

    if(0 > setsockopt(*socket_ptr, SOL_SOCKET, SO_RCVTIMEO,  &socket_timeout, sizeof(socket_timeout)))
    {
        perror("Failed to set socket timeout");
        printf("Restarting in 1 second...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    printf("Created new client socket (FD: %d).\n", client_socket);
}

static void connect_to_hub_server(void)
{
    static const uint8_t connection_error_threshold = 30;
    static uint8_t connection_error_count = 0;

    if (connection_error_count > 0)
    {
        printf("Retrying connection to Hub Intercom Server (count: %d) in 1 second...\n", connection_error_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else
    {
        printf("Attempting connection to Hub Intercom Server.\n");
    }

    if (client_socket > 0)
    {
        printf("Closing previous client socket (FD: %d).\n", client_socket);
        close(client_socket);
        client_socket = -1;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    struct in_addr server_addr_bin;
    parse_address(HUB_SERVER_IP, &server_addr_bin);
    server_addr = init_server_socket_address(server_addr_bin, htons(HUB_SERVER_PORT));
    init_local_data_socket(&client_socket, &client_addr);

    int ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (ret != 0)
    {
        perror("Failed to connect to Hub Intercom Server");
        connection_error_count++;

        if (connection_error_count > connection_error_threshold)
        {
            printf("Too many failures, restarting in 1 second.\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    connection_error_count = 0;
    client_state = CLIENTSTATE_CONNECTED;
    printf("Successfully connected to Hub Intercom Server.\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    client_send_request(PACKET_REQUEST_SYNC_TIME, 0);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("Connecting to the AP failed, retrying.\n");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        printf("Connecting to the AP succeeded.\n");
        if (client_state < CLIENTSTATE_CONNECTING) client_state = CLIENTSTATE_CONNECTING;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("got ip: " IPSTR " \n", IP2STR(&event->ip_info.ip));

        esp_netif_ip_info_t ip_info = client_get_ip_info();
        printf("My IP: " IPSTR "\n", IP2STR(&ip_info.ip));
        printf("My GW: " IPSTR "\n", IP2STR(&ip_info.gw));
        printf("My NETMASK: " IPSTR "\n", IP2STR(&ip_info.netmask));
    }
}

static void client_init(void)
{
    printf("Initializing Intercom Client task.\n");

    door_list.count = 0;
    memset(door_list.indices, (uint8_t)102, CLIENT_MAX_DOOR_COUNT);
    door_list.lock = xSemaphoreCreateMutex();
    xSemaphoreGive(door_list.lock);

    client_state = CLIENTSTATE_INIT;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &event_handler,
                                        NULL,
                                        &instance_any_id));

    ESP_ERROR_CHECK(
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &event_handler,
                                        NULL,
                                        &instance_got_ip));

    printf("Starting WiFi functionality.\n");
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Calling initial connection to Hub Access Point (SSID: %s).\n", HUB_AP_SSID);
    wifi_ap_connect(HUB_AP_SSID, HUB_AP_PASS);
}

static void process_request(void)
{
    printf("Processing request from server.\n");

    switch(rx_packet_ptr->body.Request.request_id)
    {
        case PACKET_REQUEST_SYNC_TIME:
            set_datetime(packet_decode_hour(rx_packet_ptr->header.time),
                        packet_decode_minutes(rx_packet_ptr->header.time),
                        packet_decode_seconds(rx_packet_ptr->header.time),
                        packet_decode_day(rx_packet_ptr->header.date),
                        packet_decode_month(rx_packet_ptr->header.date),
                        packet_decode_year(rx_packet_ptr->header.date));
            break;
        case PACKET_REQUEST_BELL:
            current_bell_idx = (int8_t)rx_packet_ptr->body.Request.source_id;
            printf("Received bell from door id %d.\n", current_bell_idx);
            client_state = CLIENTSTATE_BELL;
            vTaskDelay(pdMS_TO_TICKS(5000));
            client_state = CLIENTSTATE_CONNECTED;
            current_bell_idx = -1;
            break;
        case PACKET_REQUEST_NONE:
        case PACKET_REQUEST_PING:
        case PACKET_REQUEST_PHOTO:
        case PACKET_REQUEST_RESET_ADDRESS:
        case PACKET_REQUEST_SYNC_PASS_USER:
        case PACKET_REQUEST_SYNC_PASS_ADMIN:
        case PACKET_REQUEST_DOOR_OPEN:
        case PACKET_REQUEST_DOOR_CLOSE:
        case PACKET_REQUEST_MAX:
        default:
            break;
    }
}

static void process_data(void)
{
    printf("Processing data from server.\n");

    switch(rx_packet_ptr->body.Data.data_type)
    {
    case PACKET_DATA_DOOR_INFO:
        printf("Received door info: index %u, name %s.\n", rx_info_ptr->index, rx_info_ptr->name);

        xSemaphoreTake(door_list.lock, portMAX_DELAY);

        int16_t target_cell = -1;

        for (int i = 0; i < door_list.count; i++)
        {
            if (door_list.indices[i] == rx_info_ptr->index)
            {
                target_cell = i;
                break;
            }
        }

        if (target_cell == -1)
        {
            target_cell = door_list.count;
            door_list.count++;
        }

        if (door_list.count >= CLIENT_MAX_DOOR_COUNT)
        {
            door_list.count--;
            printf("Door list full, discarding new info.\n");
        }
        else
        {
            door_list.indices[target_cell] = rx_info_ptr->index;
            strncpy(door_list.names[target_cell], rx_info_ptr->name, UNIT_NAME_MAX_LEN);
            printf("New info added to door list.\n");
            gui_update_door_button(target_cell, true, door_list.names[target_cell]);
        }

        xSemaphoreGive(door_list.lock);
        break;
    case PACKET_DATA_IMAGE:
        printf("Received image data.\n");
        break;
    case PACKET_DATA_NONE:
    case PACKET_DATA_CLIENT_INFO:
    case PACKET_DATA_MAX:
      break;
    }
}


static int receive_packet()
{
    int ret = recv(client_socket, rx_buff, sizeof(rx_buff), 0);

    if (ret > 0)
    {
        switch(rx_packet_ptr->header.category)
        {
            case PACKET_CAT_REQUEST:
                process_request();
                break;
            case PACKET_CAT_DATA:
                process_data();
                break;
            case PACKET_CAT_REPORT:
            case PACKET_CAT_MAX:
            case PACKET_CAT_NONE:
              break;
        }
    }

    explicit_bzero(rx_buff, sizeof(rx_buff));

    return ret;
}

static void hub_comms(void)
{
    static const uint8_t connection_error_threshold = 10;
    static uint8_t connection_error_count = 0;

#define HANDLE_RET(ret) \
    if (ret > 0) { \
        connection_error_count = 0; \
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) { \
    } else { \
        connection_error_count++; \
        if (connection_error_count > connection_error_threshold) { \
            perror("Socket TX/RX fail"); \
            client_state = CLIENTSTATE_CONNECTING; \
            vTaskDelay(pdMS_TO_TICKS(500)); \
            return; \
        } \
    } 

    HANDLE_RET(receive_packet());

#undef HANDLE_RET
}

static void client_loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(10));

    client_state_out = client_state;

    switch(client_state)
    {
    case CLIENTSTATE_NONE:
    case CLIENTSTATE_INIT:
    default:
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
    case CLIENTSTATE_CONNECTING:
        connect_to_hub_server();
        break;
    case CLIENTSTATE_CONNECTED:
    case CLIENTSTATE_BELL:
        hub_comms();
        break;
    }
}

ClientState_t client_get_state(void)
{
    return client_state_out;
}

void client_task(void *arg)
{
    client_init();

    for(;;)
    {
        client_loop();
    }
}
