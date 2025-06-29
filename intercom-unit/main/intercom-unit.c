/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "common.h"
#include "wifi.h"
#include "gfx.h"
#include "gui.h"
#include "client.h"

#define GUI_STACK_SIZE 4096
#define CLIENT_STACK_SIZE 4096

static StaticTask_t gui_task_buffer;
static TaskHandle_t gui_task_handle = NULL;
static StackType_t gui_task_stack[GUI_STACK_SIZE];

static StaticTask_t client_task_buffer;
static TaskHandle_t client_task_handle = NULL;
static StackType_t client_task_stack[CLIENT_STACK_SIZE];

void app_main(void)
{
    /* Print chip information */
    /*
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    */

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init();
    printf("Free heap size after Wi-Fi init: %" PRIu32 " bytes\n", esp_get_free_heap_size());
    gfx_init(LCD_LANDSCAPE);
    printf("Free heap size after Gfx init: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    client_task_handle = xTaskCreateStaticPinnedToCore(client_task, "Client_Task", CLIENT_STACK_SIZE, NULL, 10, client_task_stack, &client_task_buffer, 0);
    gui_task_handle = xTaskCreateStaticPinnedToCore(gui_task, "GUI_Task", GUI_STACK_SIZE, NULL, 10, gui_task_stack, &gui_task_buffer, 1);

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /*
    for (int i = 1; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
    */
}
