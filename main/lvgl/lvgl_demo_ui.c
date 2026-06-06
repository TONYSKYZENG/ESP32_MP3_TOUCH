/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#loader-with-arc

#include "lvgl.h"
#include <stdint.h>
#include "esp_err.h"
#include "sdcard_list.h"
#include "sdcard_scan.h"
#include <string.h>
#include <stdatomic.h>
#include "play_embeded.h"
static lv_obj_t * btn,*slider_vol,*music_roller,*btn_loop,*btn_loop_text,*btn_source,*btn_source_text;
static lv_display_rotation_t rotation = LV_DISP_ROTATION_270;
extern int player_volume;
extern void set_player_vol(int vol);
static _lock_t lvgl_window_lock;
#define MAX_LIST_BUF_SIZE  (8192) // 根据最大歌曲数量预估内存（4KB 足够存约 100-200 首英文歌名）
static char * song_string_buf = NULL;
extern char * song_string_emb;
extern void play_music_index(int idx);
atomic_int g_is_loop_play = 0;
int get_loop_play(void) {
    int current_status = atomic_load(&g_is_loop_play);
    return current_status;
}
void set_loop_play(int val) {
    atomic_store(&g_is_loop_play, val);
}
static void btn_cb(lv_event_t * e)
{
    int selected_index = lv_roller_get_selected(music_roller);
    play_music_index(selected_index);
}
static void btn_loop_cb(lv_event_t * e)
{
    int is_loop = get_loop_play();
    if(is_loop) {
        set_loop_play(0);
        lv_label_set_text_static(btn_loop_text, "->");
    }
    else {
         set_loop_play(1);
        lv_label_set_text_static(btn_loop_text, LV_SYMBOL_LOOP" ");
    }
}
static void btn_source_cb(lv_event_t * e)
{
    int curr_status = get_play_source();
    if(curr_status==0) {
        lv_label_set_text_static(btn_source_text, "EMB");
        set_play_source(1);
        play_embeded_loop(mp3_data_start_train,mp3_data_end_train);
    }
    else {
         lv_label_set_text_static(btn_source_text, "SD");
        set_play_source(0);
    }
    
}
void action_on_vol_value_changed(lv_event_t * e){
    lv_obj_t * slider = lv_event_get_target(e);
    
    // 2. 获取 Slider 当前的数值
    int32_t value = lv_slider_get_value(slider);
    _lock_acquire(&lvgl_window_lock);
    set_player_vol(value);
    _lock_release(&lvgl_window_lock);
}
static void set_angle(void * obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

void example_lvgl_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_disp_set_rotation(disp, LV_DISP_ROTATION_270);
   

    /*Button event*/
   
    {
            // btn_go
            lv_obj_t *obj = lv_button_create(scr);
            btn = obj;
            lv_obj_set_pos(obj, 19, 180);
            lv_obj_set_size(obj, 71, 26);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, LV_SYMBOL_AUDIO" Go");
                }
            }
            lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, disp);
        }
    {
            // btn_loop
            lv_obj_t *obj = lv_button_create(scr);
            btn_loop = obj;
            lv_obj_set_pos(obj, 105, 180);
            lv_obj_set_size(obj, 71, 26);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    btn_loop_text = obj;
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "->");
                }
            }
            lv_obj_add_event_cb(btn_loop, btn_loop_cb, LV_EVENT_CLICKED, disp);
        }
    /*vol slider*/
     {
            // slider_vol
            lv_obj_t *obj = lv_slider_create(scr);
            slider_vol = obj;
            lv_obj_set_pos(obj, 20, 14);
            lv_obj_set_size(obj, 120, 16);
            lv_slider_set_value(obj, 50, LV_ANIM_ON);
            lv_obj_add_event_cb(obj, action_on_vol_value_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
     }
   {
            // music_roller
            lv_obj_t *obj = lv_roller_create(scr);
            music_roller = obj;
            lv_obj_set_pos(obj, 25, 54);
            lv_obj_set_size(obj, 180, 100);
            song_string_buf = (char *)malloc(MAX_LIST_BUF_SIZE);
            lv_roller_set_visible_row_count(music_roller, 3);
        if (song_string_buf) {
            song_string_buf[0] = '\0'; // 初始化为空字符串
        }
    }
    {
            // btn_source
            lv_obj_t *obj = lv_button_create(scr);
            btn_source = obj;
            lv_obj_set_pos(obj, 180, 180);
            lv_obj_set_size(obj, 71, 26);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    btn_source_text = obj;
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "SD");
                }
            }
            lv_obj_add_event_cb(btn_source, btn_source_cb, LV_EVENT_CLICKED, disp);
    }
}
void sync_vol(int player_volume){
     _lock_acquire(&lvgl_window_lock);
    lv_slider_set_value(slider_vol, player_volume, LV_ANIM_OFF);
     _lock_release(&lvgl_window_lock);
}
// 每扫描到一首歌，往缓冲区里追加 "歌名\n"
void on_each_song_scanned(const char * filename) {
    if (song_string_buf == NULL) return;
    
    // 检查防止缓冲区溢出
    if (strlen(song_string_buf) + strlen(filename) + 2 < MAX_LIST_BUF_SIZE) {
        // 如果不是第一首歌，先补一个换行符
        if (song_string_buf[0] != '\0') {
            strcat(song_string_buf, "\n");
        }
        strcat(song_string_buf, filename); // 拼接歌名
    }
}
void update_song_list_sd(playlist_operator_handle_t handle){
    _lock_acquire(&lvgl_window_lock);

    int songs = sdcard_list_get_url_num(handle);
    char *url = NULL;
    for (int i=0;i<songs;i++) {
        sdcard_list_choose( handle, i, &url);
        on_each_song_scanned(&url[14]);
    }
    lv_roller_set_options(music_roller, song_string_buf, LV_ROLLER_MODE_NORMAL);

    _lock_release(&lvgl_window_lock);
}
void update_song_list_emb(void){
    _lock_acquire(&lvgl_window_lock);
    lv_roller_set_options(music_roller, song_string_emb, LV_ROLLER_MODE_NORMAL);
    _lock_release(&lvgl_window_lock);
}
