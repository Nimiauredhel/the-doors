idf_component_register(SRC_DIRS "." "lcd" "utils" "gfx" "audio" "gui" "wifi"
                    PRIV_REQUIRES spi_flash esp_wifi
                    INCLUDE_DIRS "." "lcd" "utils" "gfx" "audio" "gui" "wifi")
