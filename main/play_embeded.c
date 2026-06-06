#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "esp_peripherals.h"
#include "board.h"
//#include "tts.h"
#include "mp3_decoder.h"
#include "play_embeded.h"

#include <stdatomic.h>
static const char *TAG = "PLAY_MP3_EMB";
extern audio_board_handle_t g_board_handle;
extern audio_pipeline_handle_t pipeline,pipeline_embeded;
audio_element_handle_t i2s_stream_writer_2,mp3_decoder_2;
extern audio_element_handle_t i2s_stream_writer;
atomic_int music_source = 0;
extern atomic_int force_music_idx;
static struct marker {
    int pos;
    const uint8_t *start;
    const uint8_t *end;
} file_marker;
const char * song_string_emb = "Train\n"
                             "Plane\n"
                             "River";
extern playlist_operator_handle_t sdcard_list_handle;
int mp3_music_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    int read_size = file_marker.end - file_marker.start - file_marker.pos;
    int file_size = file_marker.end - file_marker.start;
    if (read_size == 0) {
        return AEL_IO_DONE;
    } else if (len < read_size) {
        read_size = len;
    }
    memcpy(buf, file_marker.start + file_marker.pos, read_size);
    file_marker.pos += read_size;
    if(file_marker.pos>=file_size){
        file_marker.pos=0;
    }
    return read_size;
}
void init_sound_embeded(void){
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline_embeded = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline_embeded);

    ESP_LOGI(TAG, "[1.1] Create mp3 decoder to decode mp3 file and set custom read callback");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder_2 = mp3_decoder_init(&mp3_cfg);
    audio_element_set_read_cb(mp3_decoder_2, mp3_music_read_cb, NULL);

    ESP_LOGI(TAG, "[1.2] Create i2s stream to write data to codec chip");
#if defined CONFIG_ESP32_C3_LYRA_V2_BOARD
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_PDM_TX_CFG_DEFAULT();
#else
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
#endif
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer_2 = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[1.3] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline_embeded, mp3_decoder_2, "mp3_2");
    audio_pipeline_register(pipeline_embeded, i2s_stream_writer_2, "i2s_2");

    ESP_LOGI(TAG, "[1.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[2] = {"mp3_2", "i2s_2"};
    file_marker.start = mp3_data_start_train;
    file_marker.end = mp3_data_end_train;
    file_marker.pos = 0;
    audio_pipeline_link(pipeline_embeded, &link_tag[0], 2);
    audio_pipeline_run(pipeline_embeded);
    audio_pipeline_pause(pipeline_embeded);
}
void play_embeded_loop(uint8_t *start,uint8_t *end){
    audio_pipeline_pause(pipeline_embeded);
    i2s_stream_set_clk(i2s_stream_writer_2, 32000, 16, 1);
    file_marker.start = start;
    file_marker.end = end;
    file_marker.pos = 0;
    audio_pipeline_resume(pipeline_embeded);
}
void back_to_sd(void) {
    audio_pipeline_pause(pipeline_embeded);
    audio_pipeline_resume(pipeline);
}
void set_play_source(int i){
    if (i==0) {
        update_song_list_sd(sdcard_list_handle);
        audio_pipeline_pause(pipeline_embeded);
        i2s_stream_set_clk(i2s_stream_writer, 48000, 16, 2);
        audio_pipeline_resume(pipeline);
    }
    else{
        audio_pipeline_pause(pipeline);
        update_song_list_emb();
        i2s_stream_set_clk(i2s_stream_writer_2, 32000, 16, 1);
        audio_pipeline_resume(pipeline_embeded);
    }
    atomic_store(&music_source, i);
}
void stop_music(void) {
    audio_pipeline_pause(pipeline_embeded);
    audio_pipeline_pause(pipeline);
}
int get_play_source(void){
    int current_status = atomic_load(&music_source);
    return current_status;
}
void play_music_index_emb(void){
    int i = atomic_load(&force_music_idx);
    switch (i)
    {
    case 0:
        play_embeded_loop(mp3_data_start_train,mp3_data_end_train);
        break;
    case 1:
        play_embeded_loop(mp3_data_start_plane,mp3_data_end_plane);
        break;
    case 2:
        play_embeded_loop(mp3_data_start_river,mp3_data_end_river);
        break;
    default:
        break;
    }
}