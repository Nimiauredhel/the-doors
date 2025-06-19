#include "client.h"

static int client_socket = -1;
static struct sockaddr_in client_addr = {0};
static struct sockaddr_in server_addr ={0};

static volatile ClientState_t client_state = CLIENTSTATE_NONE;

static DoorPacket_t request_rx_buff = {0};
static uint8_t data_rx_buff[sizeof(DoorPacket_t) + DOOR_DATA_BYTES_LARGE] = {0};

struct sockaddr_in init_server_socket_address(struct in_addr peer_address_bin, in_port_t peer_port_bin)
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

        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_info);
        printf("My IP: " IPSTR "\n", IP2STR(&ip_info.ip));
        printf("My GW: " IPSTR "\n", IP2STR(&ip_info.gw));
        printf("My NETMASK: " IPSTR "\n", IP2STR(&ip_info.netmask));
    }
}

static void client_init(void)
{
    printf("Initializing Intercom Client task.\n");

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

int send_request(DoorRequest_t request, uint16_t destination)
{
    DoorPacket_t tx_buff =
    {
        // TODO: include date/time, source, version, priority etc
        .header.category = PACKET_CAT_REQUEST,
        .body.Request.destination_id = destination,
        .body.Request.request_id = request,
    };

    return send(client_socket, &tx_buff, sizeof(tx_buff), 0);
}

void process_request(void)
{
    switch(request_rx_buff.body.Request.request_id)
    {
        case PACKET_REQUEST_BELL:
            printf("Received bell, sending back a Door Open request.\n");
            send_request(PACKET_REQUEST_DOOR_OPEN, request_rx_buff.body.Request.source_id);
            break;
        case PACKET_REQUEST_NONE:
        case PACKET_REQUEST_PING:
        case PACKET_REQUEST_PHOTO:
        case PACKET_REQUEST_RESET_ADDRESS:
        case PACKET_REQUEST_SYNC_PASS_USER:
        case PACKET_REQUEST_SYNC_PASS_ADMIN:
        case PACKET_REQUEST_SYNC_TIME:
        case PACKET_REQUEST_DOOR_OPEN:
        case PACKET_REQUEST_DOOR_CLOSE:
        case PACKET_REQUEST_MAX:
        default:
            break;
    }
}

int receive_request()
{
    printf("Receiving from server.\n");

    int ret = recv(client_socket, &request_rx_buff, sizeof(request_rx_buff), 0);

    if (ret > 0)
    {
        process_request();
    }

    explicit_bzero(&request_rx_buff, sizeof(request_rx_buff));

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
        printf(" ."); \
    } else { \
        connection_error_count++; \
        if (connection_error_count > connection_error_threshold) { \
            perror("Socket TX/RX fail"); \
            client_state = CLIENTSTATE_CONNECTING; \
            vTaskDelay(pdMS_TO_TICKS(500)); \
            return; \
        } \
    } 

#undef HANDLE_RET
}

static void client_loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(10));

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
        hub_comms();
        break;
    }
}

void client_task(void *arg)
{
    client_init();

    for(;;)
    {
        client_loop();
    }
}
