set(COMPONENT_SRCS "play_sdcard_mp3_control_example.c" "play_embeded.c" "aht10.c"
                    "lvgl/spi_lcd_touch_example_main.c"
                    "lvgl/lvgl_demo_ui.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_EMBED_TXTFILES train.mp3 plane.mp3 river.mp3)
register_component()