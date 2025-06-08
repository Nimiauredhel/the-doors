#include "wifi.h"

wifi_config_t wifi_cfg = {0}; 

void wifi_init(void)
{
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));
}

void wifi_ap_connect(char *ssid, char *pass)
{
    memcpy(wifi_cfg.sta.ssid, ssid, 32);
    memcpy(wifi_cfg.sta.password, pass, 64);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);

    ESP_ERROR_CHECK(esp_wifi_connect());
}

