#include "client.h"

static void client_init(void)
{
    wifi_ap_connect("SSID", "PASSWORD");
}

static void client_loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("Hello this is client.\n");
}

void client_task(void *arg)
{
    client_init();

    for(;;)
    {
        client_loop();
    }
}
