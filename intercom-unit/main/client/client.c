#include "client.h"

static int client_socket = -1;
static struct sockaddr_in client_addr = {0};
static struct sockaddr_in server_addr ={0};

static volatile bool socket_created = false;
static volatile ClientState_t client_state = CLIENTSTATE_NONE;

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
    //static const struct timeval socket_timeout = { .tv_sec = 1, .tv_usec = 0 };

    *socket_ptr = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (*socket_ptr < 0)
    {
        perror("Failed to create data socket");
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

    if(0 > setsockopt(*socket_ptr, SOL_SOCKET, SO_RCVTIMEO,  &socket_timeout, sizeof(socket_timeout)))
    {
        perror("Failed to set socket timeout");
        esp_restart();
    }
    */

    printf("Created client socket.\n");
}

static void client_connect(void)
{
    if (!socket_created)
    {
        socket_created = true;
        struct in_addr server_addr_bin;
        parse_address("192.168.8.1", &server_addr_bin);
        server_addr = init_server_socket_address(server_addr_bin, htons(SERVER_PORT));
        init_local_data_socket(&client_socket, &client_addr);
    }

    int ret = connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (ret != 0)
    {
        perror("Failed to connect to Hub, retrying next loop");
        return;
    }

    client_state = CLIENTSTATE_ONLINE;
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
    client_state = CLIENTSTATE_INIT;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_ap_connect("DOORS-HUB", "mAlabama");
}

static void client_loop(void)
{
    int ret;
    char* msg = "Hello this is client.";

    switch(client_state)
    {
    case CLIENTSTATE_NONE:
    case CLIENTSTATE_INIT:
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
    case CLIENTSTATE_CONNECTING:
        client_connect();
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
    case CLIENTSTATE_ONLINE:
        ret = send(client_socket, msg, strlen(msg)+1, 0);

        if (ret > 0)
        {
            char rx_buff[64] = {0};
            printf("Sent message: %s\n", msg);

            ret = recv(client_socket, rx_buff, sizeof(rx_buff), 0);

            if (ret > 0)
            {
                printf("Received reply: %s\n", rx_buff);
            }

            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        else
        {
            client_state = CLIENTSTATE_CONNECTING;
            perror("Failed to send message");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
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
