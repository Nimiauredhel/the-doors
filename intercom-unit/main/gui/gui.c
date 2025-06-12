#include "gui.h"

static void gui_init(void)
{
    gui_gfx_init();
    vTaskDelay(pdMS_TO_TICKS(16));
    gui_touch_init();
    vTaskDelay(pdMS_TO_TICKS(16));
}

static void gui_loop(void)
{
    gui_touch_update();
    gui_gfx_loop();
    vTaskDelay(pdMS_TO_TICKS(32));
}

void gui_task(void *arg)
{
    gui_init();
    printf("Free heap size after GUI init: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    for(;;)
    {
        gui_loop();
    }
}

