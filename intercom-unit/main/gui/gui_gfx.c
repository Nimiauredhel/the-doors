#include "gui_gfx.h"

static uint8_t status_bar_buffer[sizeof(GfxWindow_t) + (320*32*2)];
static uint8_t info_window_buffer[sizeof(GfxWindow_t) + (320*88*2)];
static GfxWindow_t *status_bar = NULL;
static GfxWindow_t *info_window = NULL;
static GfxWindow_t *input_window = NULL;

static void gui_gfx_draw_logo(void)
{
    gfx_print_string("DOORS", 10, 42, color_red, 8);
    gfx_print_string("DOORS", 5, 36, color_white, 8);
    gfx_print_string("DOORS", 9, 38, color_black, 8);
}

static void gui_gfx_draw_input_layout(const InterfaceInputElement_t *layout, int8_t touched_button_idx)
{
    static const Color565_t *color_bg_neutral = &color_grey_dark;
    static const Color565_t *color_bg_pressed = &color_black;
    static const Color565_t *color_bg_disabled = &color_grey_light;
    static const Color565_t *color_bg_bell = &color_yellow;
    static const Color565_t *color_border = &color_blue;
    static const Color565_t *color_label_neutral = &color_green;
    static const Color565_t *color_label_pressed = &color_black;
    static const Color565_t *color_label_disabled = &color_grey_mid;
    static const Color565_t *color_label_bell = &color_red;
    static const Color565_t *color_icon_neutral = &color_white;
    static const Color565_t *color_icon_pressed = &color_blue;
    static const Color565_t *color_icon_disabled = &color_grey_dark;
    static const Color565_t *color_icon_bell = &color_magenta;

    if (layout == NULL) return;

    int8_t bell_idx = layout->id == ILAYOUT_DOOR_LIST ? client_get_current_bell_idx() : -1;

    for (int i = 0; i < layout->button_count; i++)
    {
        gfx_fill_rect_single_color(layout->buttons[i].x, layout->buttons[i].y, layout->buttons[i].width, layout->buttons[i].height, *color_border);
        gfx_fill_rect_single_color(layout->buttons[i].x+1, layout->buttons[i].y+1, layout->buttons[i].width-2, layout->buttons[i].height-2, 
                     bell_idx == i ? *color_bg_bell : touched_button_idx == i ? *color_bg_pressed : *color_bg_neutral);


        if(layout->buttons[i].icon != NULL)
        {
            gfx_draw_binary_sprite(layout->buttons[i].icon,
                    layout->buttons[i].x+(layout->buttons[i].width/2)-((layout->buttons[i].icon->width_bytes*8*layout->buttons[i].label_scale)/2),
                    layout->buttons[i].y+((layout->buttons[i].height/2)-((layout->buttons[i].icon->height_pixels*layout->buttons[i].label_scale)/2)),
                    bell_idx == i ? *color_icon_bell : touched_button_idx == i ? *color_icon_pressed : *color_icon_neutral, layout->buttons[i].label_scale);
        }

        gfx_print_string((char *)layout->buttons[i].label,
                layout->buttons[i].x+((layout->buttons[i].width/2)-(strlen(layout->buttons[i].label)*5*layout->buttons[i].label_scale)/2),
                layout->buttons[i].y+((layout->buttons[i].height/2)-(6*layout->buttons[i].label_scale)),
                bell_idx == i ? *color_label_bell : touched_button_idx == i ? *color_label_pressed : *color_label_neutral, layout->buttons[i].label_scale);
    }
}

void gui_gfx_init(void)
{
    printf("%u\n", icon_open.width_bytes);
    status_bar = gfx_create_window_nonalloc(0, 0, 320, 32, "Status", (GfxWindow_t *)&status_bar_buffer);
    gfx_show_window(status_bar);
    info_window = gfx_create_window_nonalloc(0, 32, 320, 88, "Info", (GfxWindow_t *)&info_window_buffer);
    gfx_show_window(info_window);
    input_window = gfx_create_window(0, 120, 320, 120, "Input");
    gfx_show_window(input_window);

    gfx_select_window(status_bar, true);
    gfx_fill_screen(color_black);
    gfx_draw_binary_sprite(&icon_wifi, 4, 4, color_grey_mid, 1);
    gfx_unselect_window(status_bar);
    gfx_select_window(info_window, true);
    gfx_fill_screen(color_magenta);
    gui_gfx_draw_logo();
    gfx_unselect_window(info_window);
    gfx_select_window(input_window, true);
    gfx_fill_screen(color_cyan);
    gui_gfx_draw_input_layout(gui_get_current_input_layout(), -1);
    gfx_unselect_window(input_window);
}

void gui_gfx_loop(void)
{
    static const uint8_t status_row1_text_y = 6;
    static const uint8_t status_row1_icon_y = 2;
    static const uint8_t status_row2_text_y = 20;
    static const uint8_t status_col1_x = 4;
    static const uint8_t status_col2_x = 24;
    static const uint8_t status_col3_x = 48;
    static const uint8_t status_col4_x = 72;
    static const uint8_t status_col5_x = 96;
    static const uint8_t status_col6_x = 120;
    static const uint8_t status_col7_x = 144;
    static const uint8_t status_col8_x = 166;

    static InterfaceInputElement_t *prev_layout = NULL;
    static int8_t last_touched_button_idx = -5;
    static int8_t last_bell_idx = -1;
    static ClientState_t last_client_state = -1;
    static struct tm last_datetime = {0};

    esp_netif_ip_info_t ip_info;
    uint8_t mac[6] = {0};
    char gui_text_buff[64] = {0};
    struct tm datetime = get_datetime();

    int8_t bell_idx = client_get_current_bell_idx();

    if (last_client_state != client_get_state()
        || datetime.tm_sec != last_datetime.tm_sec
        || last_bell_idx != bell_idx)
    {
        gfx_select_window(status_bar, true);
        gfx_fill_screen(color_black);
        gfx_print_string("Client Name", status_col3_x, status_row1_text_y, color_blue, 1);

        esp_netif_get_mac(esp_netif_get_default_netif(), mac);
        sprintf(gui_text_buff, "%02x:%02x:%02x:%02x:%02x:%02x",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
        gfx_print_string(gui_text_buff, status_col1_x, status_row2_text_y, color_blue, 1);


        last_client_state = client_get_state();

        switch(last_client_state)
        {
            case CLIENTSTATE_NONE:
            default:
                gfx_draw_binary_sprite(&icon_wifi, status_col1_x, status_row1_icon_y, color_grey_mid, 1);
                break;
            case CLIENTSTATE_INIT:
                gfx_draw_binary_sprite(&icon_wifi, status_col1_x, status_row1_icon_y, color_red_dark, 1);
                break;
            case CLIENTSTATE_CONNECTING:
                gfx_draw_binary_sprite(&icon_wifi, status_col1_x, status_row1_icon_y, color_red, 1);
                break;
            case CLIENTSTATE_CONNECTED:
            case CLIENTSTATE_BELL:
                gfx_draw_binary_sprite(&icon_wifi, status_col1_x, status_row1_icon_y, color_green, 1);
                gfx_draw_binary_sprite(&icon_bell, status_col2_x, status_row1_icon_y-2,
                        last_client_state == CLIENTSTATE_BELL ? color_yellow : color_grey_light, 1);

                last_datetime = datetime;
                sprintf(gui_text_buff, "%02u:%02u:%02u %02u/%02u/%04u",
                        last_datetime.tm_hour, last_datetime.tm_min, last_datetime.tm_sec,
                        last_datetime.tm_mday, last_datetime.tm_mon, last_datetime.tm_year);
                gfx_print_string(gui_text_buff, status_col8_x, status_row1_text_y, color_cyan, 1);

                ip_info = client_get_ip_info();
                sprintf(gui_text_buff, IPSTR, IP2STR(&ip_info.ip));
                gfx_print_string(gui_text_buff, status_col8_x, status_row2_text_y, color_cyan, 1);

                break;
        }
        gfx_unselect_window(status_bar);
    }

    int8_t button_idx = gui_get_touched_button_idx();
    InterfaceInputElement_t *layout = gui_get_current_input_layout();

    if (button_idx != last_touched_button_idx
        || layout != prev_layout
        || bell_idx != last_bell_idx)
    {
        gfx_select_window(input_window, true);
        gfx_fill_screen(color_cyan);
        gui_gfx_draw_input_layout(layout, button_idx);
        gfx_unselect_window(input_window);
    }

    last_touched_button_idx = button_idx;
    last_bell_idx = bell_idx;
    prev_layout = layout;
}
