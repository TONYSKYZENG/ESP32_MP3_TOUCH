#ifndef _PLAY_EMBEDED_H_
#define _PLAY_EMBEDED_H_
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdcard_list.h"
#include "sdcard_scan.h"

/*void startMusic(void);
void i2s_music_mp3(void *args);
void i2s_music_mp3_once (void *args);
*/

extern const uint8_t mp3_data_start_train[] asm("_binary_train_mp3_start");
extern const uint8_t mp3_data_end_train[] asm("_binary_train_mp3_end");

extern const uint8_t mp3_data_start_plane[] asm("_binary_plane_mp3_start");
extern const uint8_t mp3_data_end_plane[] asm("_binary_plane_mp3_end");

extern const uint8_t mp3_data_start_river[] asm("_binary_river_mp3_start");
extern const uint8_t mp3_data_end_river[] asm("_binary_river_mp3_end");
void init_sound_embeded(void);
void stop_sound_embeded(void);
void set_play_source(int i);
int get_play_source(void);

void set_timer_max(int i);
int get_timer_max(void);

void set_timer_cur(int i);
int get_timer_cur(void);

void play_embeded_loop(uint8_t *start,uint8_t *end);
void update_song_list_sd(playlist_operator_handle_t handle);
void update_song_list_emb(void);
void play_music_index_sd(void);
void play_music_index_emb(void);
void stop_music(void);
void update_time_show(void);
#endif