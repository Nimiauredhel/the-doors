#include "gui.h"

static esp_lcd_touch_handle_t touch_handle = NULL;
static volatile InterfaceTouchState_t touch_state = {0};

static GfxWindow_t *gui_window = NULL;

static void gui_gfx_init(void)
{
    gfx_init(LCD_LANDSCAPE);
    gui_window = gfx_create_window(0, 0, 320, 240, "GUI");
    gfx_show_window(gui_window);
}

static void gui_touch_init(void)
{
     esp_lcd_touch_config_t tp_cfg = {
        .x_max = 320,
        .y_max = 240,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_LOGI("GUI", "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(io_handle, &tp_cfg, &touch_handle));
    gfx_set_touch_handle(touch_handle);
}

static bool gui_touch_update(void)
{
    uint16_t x[3];
    uint16_t y[3];
    uint16_t strength[3];
    uint8_t count = 0;

    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, x, y, strength, &count, 3);

    if (touchpad_pressed)
    {
        touch_state.current_x = x[0];
        touch_state.current_y = y[0];
        touch_state.current_z = strength[0];

        for (int i = 0; i < count; i++)
        {
            printf("Touch Detected at ");
            printf("X: %u Y: %u Z: %u | ", x[i], y[i], strength[i]);
        }
        printf("\n");
    }

    return touchpad_pressed;
}

void gui_init(void)
{
    gui_gfx_init();
    gui_touch_init();

    gfx_select_window(gui_window, true);
    gfx_fill_screen(color_magenta);
    gfx_unselect_window(gui_window);
}

void gui_loop(void)
{
    if (gui_touch_update())
    {
        gfx_select_window(gui_window, true);
        gfx_fill_screen(color_black);
        gfx_fill_rect_single_color(touch_state.current_x, touch_state.current_y, 32, 24, color_red);
        gfx_unselect_window(gui_window);
    }
    vTaskDelay(pdMS_TO_TICKS(32));
}
