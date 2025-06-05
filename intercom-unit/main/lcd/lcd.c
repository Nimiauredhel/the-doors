#include "lcd.h"

#define LCD_HOST 1

// bus variables
spi_bus_config_t buscfg =
{
    .sclk_io_num = 17,  // IO number for connecting LCD SCK (SCL) signal
    .mosi_io_num = 16,  // IO number for connecting LCD MOSI (SDO, SDA) signal
    .miso_io_num = 18,  // IO number for connecting LCD MISO (SDI) signal; set to `-1` if data read from LCD is not required
    .quadwp_io_num = -1,                  // Must be set to `-1`
    .quadhd_io_num = -1,                  // Must be set to `-1`
    .max_transfer_sz = 240 * 320 * sizeof(uint16_t), // Represents the maximum number of bytes allowed for a single SPI transfer; usually set to the screen size
};

// interface variables
esp_lcd_panel_io_handle_t io_handle = NULL;

esp_lcd_panel_io_spi_config_t io_config =
{
    .dc_gpio_num = 4,     // IO number connected to the LCD DC (RS) signal; set to `-1` to disable
    .cs_gpio_num = 15,     // IO number connected to the LCD CS signal; set to `-1` to disable
    .pclk_hz = SPI_MASTER_FREQ_40M,     // SPI clock frequency (Hz), ESP supports up to 80M (SPI_MASTER_FREQ_80M)
                                               // Determine the maximum value based on the LCD driver IC data sheet
    .lcd_cmd_bits = 8,      // Number of bits per LCD command, should be a multiple of 8
    .lcd_param_bits = 8,  // Number of bits per LCD parameter, should be a multiple of 8
    .spi_mode = 0,                             // SPI mode (0-3); determine based on the LCD driver IC data sheet and hardware configuration (e.g., IM[3:0])
    .trans_queue_depth = 10,                   // Queue depth for SPI device data transmission; usually set to 10
    .on_color_trans_done = NULL,   // Callback function after a single call to `esp_lcd_panel_draw_bitmap()` completes transmission
    .user_ctx = NULL, //&example_user_ctx,             // User parameter passed to the callback function
                                               
    // Parameters related to SPI timing; determine based on the LCD driver IC data sheet and hardware configuration
    .flags =
    {
        // Read and write data through one data line (MOSI); 0: Interface I type, 1: Interface II type
        .sio_mode = 0,
    },
};

// lcd variables
esp_lcd_panel_handle_t panel_handle = NULL;
// const gc9a01_vendor_config_t vendor_config = {  // Used to replace the initialization commands and parameters in the driver component
//     .init_cmds = lcd_init_cmds,
//     .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
// };
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = 2,                         // Connect the IO number of the LCD reset signal, set to `-1` to indicate not using
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,   // Element order of pixel color (RGB/BGR),
                                                  // Usually controlled by the command `LCD_CMD_MADCTL (36h)`
    .bits_per_pixel = 16,                         // Bit depth of the color format (RGB565: 16, RGB666: 18),
                                                  // usually controlled by the command `LCD_CMD_COLMOD (3Ah)`
    // .vendor_config = &vendor_config,           // Used to replace the initialization commands and parameters in the driver component
};

// portrait dimensions - flip when landscape
static uint16_t lcd_width = 240;
static uint16_t lcd_height = 320;

void lcd_initialize_bus(void)
{
    printf("Initializing SPI & DMA bus for LCD.\n");
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // The 1st parameter represents the SPI host ID used, consistent with subsequent interface device creation
    // The 3rd parameter represents the DMA channel number used, set to `SPI_DMA_CH_AUTO` by default
}

void lcd_initialize_interface(esp_lcd_panel_io_color_trans_done_cb_t tx_done_cb, void *user_ctx)
{
    printf("Initializing SPI interface for LCD.\n");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    /* The following functions can also be used to register the callback function for color data transmission completion events */
    if (tx_done_cb != NULL)
    {
        const esp_lcd_panel_io_callbacks_t cbs = {
            .on_color_trans_done = tx_done_cb,
        };

        esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, user_ctx);
    }
}

esp_lcd_panel_handle_t lcd_initialize_screen(LCDOrientation_t orientation)
{
    printf("Initializing LCD screen handle.\n");
    /* Create the LCD device */
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    lcd_width = orientation > 1 ? 320 : 240;
    lcd_height = orientation > 1 ? 240 : 320;

    /* Initialize the LCD device */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));   // Use these functions as needed
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, orientation == 1, orientation == 3));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, orientation > 1));
    // ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    return panel_handle;
}

void lcd_deinitialize_screen(void)
{
    esp_lcd_panel_del(panel_handle);
}

uint16_t lcd_get_width(void) { return lcd_width; }
uint16_t lcd_get_height(void) { return lcd_height; }
