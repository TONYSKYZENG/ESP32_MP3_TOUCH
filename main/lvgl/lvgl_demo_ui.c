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
#include "esp_timer.h"
#include "i2c_bus.h"
#include "aht10.h"
static lv_obj_t * btn,*slider_vol,*music_roller,*btn_loop,*btn_loop_text,*btn_source,*btn_source_text,*hour_roller,*minute_roller,*btn_time_set,*label_time_set,*label_aht;
static lv_display_rotation_t rotation = LV_DISP_ROTATION_270;
extern int player_volume;
extern void set_player_vol(int vol);
static _lock_t lvgl_window_lock;
#define MAX_LIST_BUF_SIZE  (8192) // 根据最大歌曲数量预估内存（4KB 足够存约 100-200 首英文歌名）
static char * song_string_buf = NULL;
char hour_string_buf[512];
char minute_string_buf[1024];
char label_time_buf[512];
char label_aht_buf[512];
extern char * song_string_emb;
extern void play_music_index(int idx);
atomic_int g_is_loop_play = 0;

atomic_int g_timer_max = 99999;
atomic_int g_timer_cur = 0;
esp_timer_handle_t music_timer,aht10_timer;
aht10_dev_t aht_sensor;
int get_loop_play(void) {
    int current_status = atomic_load(&g_is_loop_play);
    return current_status;
}
void set_loop_play(int val) {
    atomic_store(&g_is_loop_play, val);
}
void set_timer_max(int i) {
    atomic_store(&g_timer_max, i);
}
int get_timer_max(void) {
    int current_status = atomic_load(&g_timer_max);
    return current_status;
}

void set_timer_cur(int i) {
    atomic_store(&g_timer_cur, i);
}
int get_timer_cur(void) {
    int current_status = atomic_load(&g_timer_cur);
    return current_status;
}

static void btn_cb(lv_event_t * e)
{
    int selected_index = lv_roller_get_selected(music_roller);
    esp_timer_stop(music_timer);
    play_music_index(selected_index);
    set_timer_cur(0);
    update_time_show();
    uint64_t period_us = 60 * 1000 * 1000; 
   ESP_ERROR_CHECK(esp_timer_start_periodic(music_timer, period_us));
}
void gen_number_idx (char *buf,int idx){
     if (buf == NULL) return;
    char tempstr[64];
    // 检查防止缓冲区溢出
    for (int i=0;i<idx;i++) {
         sprintf(tempstr,"%d",i);
        if (buf[0] != '\0') {
            strcat(buf, "\n");
        }
        strcat(buf, tempstr); // 拼接歌名
    }
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
void update_time_show(void) {
    sprintf(label_time_buf,"%d:%d",get_timer_cur(),get_timer_max());
     _lock_acquire(&lvgl_window_lock);
     lv_label_set_text_static(label_time_set, label_time_buf);
     _lock_release(&lvgl_window_lock);
}
static void btn_time_set_cb(lv_event_t * e)
{   esp_timer_stop(music_timer);
    int hour_index = lv_roller_get_selected(hour_roller);
    int min_index = lv_roller_get_selected(minute_roller);
   int val_set = hour_index*60+min_index;
   set_timer_cur(0);
   set_timer_max(val_set);
   update_time_show();
   uint64_t period_us = 60 * 1000 * 1000; 
   ESP_ERROR_CHECK(esp_timer_start_periodic(music_timer, period_us));
   //update_time_show();
}
static void music_timer_callback(void* arg)
{
   int val = get_timer_cur();
   int max = get_timer_max();
   if(val<max) {
    val++;
    set_timer_cur(val);
    update_time_show();
    if(val>=max) {
            stop_music();
        }
   }
   // printf("time out: %lld ms", time_since_boot);
}

static void aht_timer_callback(void* arg)
{
    if (aht10_read_measurements(&aht_sensor) == ESP_OK) {
        sprintf(label_aht_buf, "T:%.1f C,H:%.1f %%", aht_sensor.temperature,aht_sensor.humidity);
        _lock_acquire(&lvgl_window_lock);
        lv_label_set_text_static(label_aht,label_aht_buf);
        _lock_release(&lvgl_window_lock);
        ESP_LOGI("AHT", "Temp: %.1f, Hum: %.1f", aht_sensor.temperature, aht_sensor.humidity);
    }
   // printf("time out: %lld ms", time_since_boot);
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
            lv_obj_set_pos(obj, 0, 54);
            lv_obj_set_size(obj, 150, 100);
            song_string_buf = (char *)malloc(MAX_LIST_BUF_SIZE);
            //lv_roller_set_visible_row_count(music_roller, 3);
        if (song_string_buf) {
            song_string_buf[0] = '\0'; // 初始化为空字符串
        }
    }
    gen_number_idx(hour_string_buf,12);
    gen_number_idx(minute_string_buf,60);
    hour_roller = lv_roller_create(scr);
    // 用 \n 把数字一行行传进去
    lv_roller_set_options(hour_roller, 
       hour_string_buf, 
        LV_ROLLER_MODE_NORMAL);
    lv_obj_set_pos(hour_roller, 160, 54);
    lv_obj_set_size(hour_roller, 50, 80);
    minute_roller = lv_roller_create(scr);
    // 用 \n 把数字一行行传进去
     lv_obj_t *lable_h = lv_label_create(scr);
        lv_obj_set_pos(lable_h, 160, 32);
        lv_obj_set_size(lable_h, 50, 16);
        lv_label_set_text_static(lable_h, "hour");

    lv_roller_set_options(minute_roller, 
        minute_string_buf, 
        LV_ROLLER_MODE_NORMAL);
    lv_obj_set_pos(minute_roller, 220, 54);
    lv_obj_set_size(minute_roller, 50, 80);
    lv_roller_set_visible_row_count(hour_roller, 2); // 设置可见行数
    lv_roller_set_visible_row_count(minute_roller, 2);
    lv_obj_t *lable_m = lv_label_create(scr);
    lv_obj_set_pos(lable_m, 220, 32);
    lv_obj_set_size(lable_m, 50, 16);
    lv_label_set_text_static(lable_m, "min");

    label_time_set  = lv_label_create(scr);
    lv_obj_set_pos(label_time_set, 0, 220);
    lv_obj_set_size(label_time_set, 100, 16);
    lv_label_set_text_static(label_time_set, "0:0");

    label_aht  = lv_label_create(scr);
    lv_obj_set_pos(label_aht, 160, 0);
    lv_obj_set_size(label_aht, 120, 16);
    lv_label_set_text_static(label_aht, "0");
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
     {
            // btn_time_set_cb
            lv_obj_t *obj = lv_button_create(scr);
            btn_time_set = obj;
            lv_obj_set_pos(obj, 180, 130);
            lv_obj_set_size(obj, 71, 26);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, LV_SYMBOL_SETTINGS" OK");
                }
            }
            lv_obj_add_event_cb(btn_time_set, btn_time_set_cb, LV_EVENT_CLICKED, disp);
    }

    { //music timer
    const esp_timer_create_args_t music_timer_args = {
        .callback = &music_timer_callback, // 绑定回调函数
        .name = "music",          // 定时器名称（用于调试）
        .skip_unhandled_events = true,        
    };
    ESP_ERROR_CHECK(esp_timer_create(&music_timer_args, &music_timer));
    uint64_t period_us = 60 * 1000 * 1000; 
    ESP_ERROR_CHECK(esp_timer_start_periodic(music_timer, period_us));
    }
    
    { 
        //aht10 timer
        i2c_master_bus_handle_t bus_handle = i2c_bus_get_master_handle(I2C_NUM_0);
        ESP_ERROR_CHECK(aht10_init(bus_handle, &aht_sensor));
        const esp_timer_create_args_t aht_timer_args = {
        .callback = &aht_timer_callback, // 绑定回调函数
        .name = "aht",          // 定时器名称（用于调试）
        .skip_unhandled_events = true,        
        };
        ESP_ERROR_CHECK(esp_timer_create(&aht_timer_args, &aht10_timer));
        uint64_t period_us = 1 * 1000 * 1000; 
        ESP_ERROR_CHECK(esp_timer_start_periodic(aht10_timer, period_us));
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
