#include "gui_gfx.h"

static uint8_t status_bar_buffer[sizeof(GfxWindow_t) + (320*20*2)];
static uint8_t info_window_buffer[sizeof(GfxWindow_t) + (320*100*2)];
static GfxWindow_t *status_bar = NULL;
static GfxWindow_t *info_window = NULL;
static GfxWindow_t *input_window = NULL;

static void gui_gfx_draw_logo(void)
{
    gfx_print_string("DOORS", 10, 42, color_red, 8);
    gfx_print_string("DOORS", 5, 36, color_white, 8);
    gfx_print_string("DOORS", 9, 38, color_black, 8);
}

static void gui_gfx_draw_keyboard(int8_t touched_button_idx)
{
    for (int i = 0; i < 40; i++)
    {
        gfx_fill_rect_single_color(touch_keyboard[i].x, touch_keyboard[i].y, touch_keyboard[i].width, touch_keyboard[i].height, color_blue);
        gfx_fill_rect_single_color(touch_keyboard[i].x+2, touch_keyboard[i].y+2, touch_keyboard[i].width-2, touch_keyboard[i].height-2, touched_button_idx == i ? color_blue : color_white);
        gfx_print_string((char *)touch_keyboard[i].label, touch_keyboard[i].x+12, touch_keyboard[i].y+7,  touched_button_idx == i ? color_yellow : color_black, touch_keyboard[i].label_scale);
    }
}

void gui_gfx_init(void)
{
    status_bar = gfx_create_window_nonalloc(0, 0, 320, 20, "Info", (GfxWindow_t *)&status_bar_buffer);
    gfx_show_window(status_bar);
    info_window = gfx_create_window_nonalloc(0, 20, 320, 100, "Info", (GfxWindow_t *)&info_window_buffer);
    gfx_show_window(info_window);
    input_window = gfx_create_window(0, 120, 320, 120, "Input");
    gfx_show_window(input_window);

    gfx_select_window(status_bar, true);
    gfx_fill_screen(color_black);
    gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_green, 1);
    gfx_unselect_window(status_bar);
    gfx_select_window(info_window, true);
    gfx_fill_screen(color_magenta);
    gui_gfx_draw_logo();
    gfx_unselect_window(info_window);
    gfx_select_window(input_window, true);
    gfx_fill_screen(color_cyan);
    gui_gfx_draw_keyboard(-1);
    gfx_unselect_window(input_window);
}

void gui_gfx_loop(void)
{
    static int8_t last_touched_button_idx = -5;
    static ClientState_t last_client_state = -1;

    if (last_client_state != client_get_state())
    {
        last_client_state = client_get_state();
        gfx_select_window(status_bar, true);
        gfx_fill_screen(color_black);
        switch(last_client_state)
        {
            case CLIENTSTATE_NONE:
            default:
                gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_blue, 1);
                break;
            case CLIENTSTATE_INIT:
                gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_red, 1);
                break;
            case CLIENTSTATE_CONNECTING:
                gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_yellow, 1);
                break;
            case CLIENTSTATE_CONNECTED:
            case CLIENTSTATE_BELL:
                gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_green, 1);
                break;
        }
        gfx_unselect_window(status_bar);
    }

    if (gui_get_touched_button_idx() != last_touched_button_idx)
    {
        last_touched_button_idx = gui_get_touched_button_idx();
        gfx_select_window(input_window, true);
        gfx_fill_screen(color_cyan);
        gui_gfx_draw_keyboard(last_touched_button_idx);
        gfx_unselect_window(input_window);
    }
}
