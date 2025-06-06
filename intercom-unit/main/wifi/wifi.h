#ifndef WIFI_H
#define WIFI_H

#include "common.h"
#include "esp_wifi.h"

void wifi_init();
void wifi_ap_connect(char *ssid, char *pass);


#endif
