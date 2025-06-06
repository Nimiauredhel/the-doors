#include "client.h"

static void client_init(void)
{
    wifi_init();
}

static void client_loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
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
