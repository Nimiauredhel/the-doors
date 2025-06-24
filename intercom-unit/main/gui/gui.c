#include "gui.h"

static const uint16_t gui_loop_delay_ms = 32;

static volatile InterfaceLayout_t input_layout_idx = ILAYOUT_DOOR_LIST;
static int8_t prev_touched_button_idx = -1;
static int8_t touched_button_idx = -1;
static uint8_t selected_door_idx = 0;

static const uint8_t touch_release_threshold = 5;
static uint8_t touch_release_counter = 0;

static void gui_handle_button_touch(const InterfaceInputElement_t *layout)
{
    InterfaceAction_t action = prev_touched_button_idx < 0 ? IACTION_NONE :  layout->buttons[prev_touched_button_idx].id;

    switch(action)
    {
    case IACTION_OPEN:
        client_send_request(PACKET_REQUEST_DOOR_OPEN, selected_door_idx);
        input_layout_idx = ILAYOUT_DOOR_LIST;
        break;
    case IACTION_BACK:
        input_layout_idx = ILAYOUT_DOOR_LIST;
        break;
    case IACTION_SELECT_DOOR_1:
    case IACTION_SELECT_DOOR_2:
    case IACTION_SELECT_DOOR_3:
    case IACTION_SELECT_DOOR_4:
    case IACTION_SELECT_DOOR_5:
    case IACTION_SELECT_DOOR_6:
    case IACTION_SELECT_DOOR_7:
    case IACTION_SELECT_DOOR_8:
        selected_door_idx = action - 1;
        input_layout_idx = ILAYOUT_DOOR_ACTIONS;
      break;
    case IACTION_NONE:
    case IACTION_RESET:
    case IACTION_CAMERA:
    default:
        break;
    }
}

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
                }

                return i;
            }
        }
    }

    if (prev_result >= 0)
    {
        prev_result = -1;
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
    static int8_t prev_touched_button_result = -1;

    gui_touch_update();
    const InterfaceInputElement_t *layout = gui_get_current_input_layout();
    int8_t touched_button_result = gui_check_button_touch(layout, 40, 0, -120);

    //printf("%d : %d\n", touched_button_result, touched_button_idx);

    if (touched_button_result < 0 && touched_button_idx >= 0)
    {
        if (prev_touched_button_result < 0
            && touch_release_counter >= touch_release_threshold)
        {
            touch_release_counter = 0;
            prev_touched_button_idx = touched_button_idx;
            touched_button_idx = touched_button_result;

            if (touched_button_idx != prev_touched_button_idx)
            {
                audio_sfx_touch_up();
                gui_handle_button_touch(layout);
            }
        }
        else
        {
            touch_release_counter++;
        }
    }
    else
    {
        prev_touched_button_idx = touched_button_idx;
        touched_button_idx = touched_button_result;
        if (touched_button_idx >= 0 && touched_button_idx != prev_touched_button_idx)
        {
            audio_sfx_touch_down();
        }
        touch_release_counter = 0;
    }

    prev_touched_button_result = touched_button_result;

    gui_gfx_loop();
    if (client_get_state() == CLIENTSTATE_BELL) audio_still_open_reminder();
    vTaskDelay(pdMS_TO_TICKS(gui_loop_delay_ms));
}

InterfaceInputElement_t* gui_get_current_input_layout(void)
{
    if (input_layout_idx < 0 || input_layout_idx >= NUM_LAYOUTS) return NULL;
    return &input_layouts[input_layout_idx];
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

