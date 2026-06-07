set(LV_BALL_DIR ballgame)
file(GLOB_RECURSE LV_BALLGAME_SOURCES ${LV_BALL_DIR}/*.c)

set(LV_FLY_DIR flygame)
file(GLOB_RECURSE LV_FLYGAME_SOURCES ${LV_FLY_DIR}/*.c)

set(COMPONENT_SRCS "play_sdcard_mp3_control_example.c" "play_embeded.c" "aht10.c"
                    "lvgl/spi_lcd_touch_example_main.c"
                    "lvgl/lvgl_demo_ui.c" ${LV_BALLGAME_SOURCES} ${LV_FLYGAME_SOURCES})
set(COMPONENT_ADD_INCLUDEDIRS . ${LV_BALL_DIR} ${LV_FLY_DIR} )
set(COMPONENT_EMBED_TXTFILES train.mp3 plane.mp3 river.mp3)
register_component()