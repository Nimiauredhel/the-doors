#include "gui.h"

static int8_t input_layout_idx = 2;
static int8_t touched_button_idx = -1;

static int8_t gui_check_button_touch(const InterfaceInputElement_t *layout, const uint8_t button_count, int16_t x_offset, int16_t y_offset)
{
    static int8_t prev_result = -1;

    if (layout != NULL && gui_touch_is_touched())
    {
        uint16_t x = gui_touch_get_x() + x_offset;
        uint16_t y = gui_touch_get_y() + y_offset;

        // TODO: this can probably be made more efficient
        for (uint8_t i = 0; i < layout->button_count; i++)
        {
            if (x > layout->buttons[i].x && x < (layout->buttons[i].x + layout->buttons[i].width)
                && y > layout->buttons[i].y && y < (layout->buttons[i].y + layout->buttons[i].height))
            {
                if (i != prev_result)
                {
                    prev_result = i;
                    audio_sfx_touch_down();
                }

                return i;
            }
        }
    }

    if (prev_result >= 0)
    {
        prev_result = -1;
        audio_sfx_touch_up();
    }

    return -1;
}

static void gui_init(void)
{
    gui_gfx_init();
    vTaskDelay(pdMS_TO_TICKS(16));
    gui_touch_init();
    vTaskDelay(pdMS_TO_TICKS(16));
    init_audio();
    audio_top_phase_jingle();
}

static void gui_loop(void)
{
    gui_touch_update();
    touched_button_idx = gui_check_button_touch(gui_get_current_input_layout(), 40, 0, -120);
    gui_gfx_loop();
    if (client_get_state() == CLIENTSTATE_BELL) audio_still_open_reminder();
    vTaskDelay(pdMS_TO_TICKS(32));
}

const InterfaceInputElement_t* gui_get_current_input_layout(void)
{
    if (input_layout_idx < 0 || input_layout_idx >= NUM_LAYOUTS) return NULL;
    return input_layouts[input_layout_idx];
}

int8_t gui_get_touched_button_idx(void)
{
    return touched_button_idx;
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

