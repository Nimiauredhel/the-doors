#include "gui_gfx.h"

static uint8_t info_window_buffer[sizeof(GfxWindow_t) + (320*120*2)];
static GfxWindow_t *info_window = NULL;
static GfxWindow_t *input_window = NULL;

void gui_gfx_init(void)
{
    info_window = gfx_create_window_nonalloc(0, 0, 320, 120, "Info", (GfxWindow_t *)&info_window_buffer);
    gfx_show_window(info_window);
    input_window = gfx_create_window(0, 120, 320, 120, "Input");
    gfx_show_window(input_window);

    gfx_select_window(info_window, true);
    gfx_fill_screen(color_magenta);
    gfx_unselect_window(info_window);
    gfx_select_window(input_window, true);
    gfx_fill_screen(color_cyan);
    gfx_unselect_window(input_window);
}

void gui_gfx_loop(void)
{
    if (gui_touch_is_touched())
    {
        gfx_select_window(info_window, true);
        gfx_fill_screen(color_black);
        gfx_fill_rect_single_color(gui_touch_get_x(), gui_touch_get_y(), 32, 24, color_red);
        gfx_unselect_window(info_window);
    }
}
