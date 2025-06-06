#include "gui.h"

static esp_lcd_touch_handle_t touch_handle = NULL;
static volatile InterfaceTouchState_t touch_state = {0};

static uint8_t info_window_buffer[sizeof(GfxWindow_t) + (320*120*2)];
static GfxWindow_t *info_window = NULL;
static GfxWindow_t *input_window = NULL;

static void gui_gfx_init(void)
{
    info_window = gfx_create_window_nonalloc(0, 0, 320, 120, "Info", (GfxWindow_t *)&info_window_buffer);
    gfx_show_window(info_window);
    input_window = gfx_create_window(0, 120, 320, 120, "Input");
    gfx_show_window(input_window);
}

static void gui_process_touch_coords(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    static const uint16_t x_raw_min = 270;
    static const uint16_t x_raw_max = 3800;
    static const uint16_t y_raw_min = 270;
    static const uint16_t y_raw_max = 3800;

    for(int i = 0; i < *point_num; i++)
    {
        //printf("raw x: %u y: %u\n", x[i], y[i]);
        if (x[i] > x_raw_max) x[i] = x_raw_max;
        else if (x[i] < x_raw_min) x[i] = x_raw_min;
        if (y[i] > y_raw_max) y[i] = y_raw_max;
        else if (y[i] < y_raw_min) y[i] = y_raw_min;
        x[i] = map_uint16(x_raw_min, x_raw_max, 0, 240, x[i]);
        y[i] = map_uint16(y_raw_min, y_raw_max, 0, 320, y[i]);
    }
}

static void gui_touch_init(void)
{
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(TOUCH_CS_PIN);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

     esp_lcd_touch_config_t tp_cfg = {
        .x_max = 240,
        .y_max = 320,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 1,
        },
        .process_coordinates = gui_process_touch_coords,
    };

    ESP_LOGI("GUI", "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &touch_handle));
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

        /*
        for (int i = 0; i < count; i++)
        {
            printf("Touch Detected at ");
            printf("X: %u Y: %u Z: %u | ", x[i], y[i], strength[i]);
        }
        printf("\n");
        */
    }

    return touchpad_pressed;
}

static void gui_init(void)
{
    gui_gfx_init();
    gui_touch_init();

    gfx_select_window(info_window, true);
    gfx_fill_screen(color_magenta);
    gfx_unselect_window(info_window);
    gfx_select_window(input_window, true);
    gfx_fill_screen(color_cyan);
    gfx_unselect_window(input_window);
}

static void gui_loop(void)
{
    if (gui_touch_update())
    {
        gfx_select_window(info_window, true);
        gfx_fill_screen(color_black);
        gfx_fill_rect_single_color(touch_state.current_x, touch_state.current_y, 32, 24, color_red);
        gfx_unselect_window(info_window);
    }
    vTaskDelay(pdMS_TO_TICKS(16));
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

