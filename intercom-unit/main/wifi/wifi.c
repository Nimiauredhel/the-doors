#include "wifi.h"

wifi_config_t wifi_cfg = {0}; 

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("Connecting to the AP failed, retrying.\n");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("got ip: " IPSTR " \n", IP2STR(&event->ip_info.ip));
    }
}

void wifi_init(void)
{
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));

    ESP_ERROR_CHECK(esp_event_loop_create_default());
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
}

void wifi_ap_connect(char *ssid, char *pass)
{
    memcpy(wifi_cfg.sta.ssid, ssid, 32);
    memcpy(wifi_cfg.sta.password, pass, 64);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);

    ESP_ERROR_CHECK(esp_wifi_connect());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_info);
    printf("My IP: " IPSTR "\n", IP2STR(&ip_info.ip));
    printf("My GW: " IPSTR "\n", IP2STR(&ip_info.gw));
    printf("My NETMASK: " IPSTR "\n", IP2STR(&ip_info.netmask));
}

