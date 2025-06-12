#include "gui_touch.h"

static esp_lcd_touch_handle_t touch_handle = NULL;
static volatile InterfaceTouchState_t touch_state = {0};

static void gui_touch_process_coords(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
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

void gui_touch_init(void)
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
        .process_coordinates = gui_touch_process_coords,
    };

    ESP_LOGI("GUI", "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &touch_handle));
    gfx_set_touch_handle(touch_handle);
}

void gui_touch_update(void)
{
    uint16_t temp_x;
    uint16_t temp_y;
    uint16_t temp_z;
    uint8_t out_count = 0;

    if (!esp_lcd_touch_get_coordinates(touch_handle, &temp_x, &temp_y, &temp_z, &out_count, 1))
    {
        touch_state.is_touched = false;
        return;
    }

    touch_state.is_touched = true;
    touch_state.last_x = temp_x;
    touch_state.last_y = temp_y;
    touch_state.last_z = temp_z;

    //printf("Touch at X%03u Y%03u Z%03u\n", touch_state.last_x, touch_state.last_y, touch_state.last_z);
}

bool gui_touch_is_touched(void)
{
    return touch_state.is_touched;
}

uint16_t gui_touch_get_x(void)
{
    return touch_state.last_x;
}

uint16_t gui_touch_get_y(void)
{
    return touch_state.last_y;
}

uint16_t gui_touch_get_z(void)
{
    return touch_state.last_z;
}
