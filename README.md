# Play MP3 Files from MicroSD Card with Playlist and Touch Screen




## Example Brief

This example demonstrates how to play MP3 files stored on a microSD card using the audio pipeline interface. The example first scans the microSD card for the MP3 files and saves the results of the scan as a playlist on the microSD card.

You can control music player through LVGL touch screen (IL9341+XPT2046)


The pipeline is as follows:

```
[sdcard] ---> fatfs_stream ---> mp3_decoder ---> resample ---> i2s_stream ---> [codec_chip]
```

## Environment Setup

## Hardware and lib Required

This example runs on the board as shown in Lyrat_touch.pdf.

I have a hacked version of ADF which is 100% compatable

```
git clone  --recursive https://github.com/TONYSKYZENG/ESP-ADF_MOD.git
cd ESP-ADF_MOD
./install.sh
```

## Build and Flash

### Default IDF Branch

This example supports IDF release/v5.0 and later branches. By default, it runs on ADF's built-in branch `$ADF_PATH/esp-idf`.

### Configuration

Prepare a microSD card with some MP3 files on the card for backup.
 
If you need to run this example on other development boards, select the board in menuconfig, such as `ESP32-Lyrat-Mini V1.1`.

```
menuconfig > Audio HAL > ESP32-Lyrat-Mini V1.1
```

Then, enable FatFs long filename support.

```
menuconfig > Component config > FAT Filesystem support > Long filename support
```

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output (replace `PORT` with your board's serial port name):

```
idf.py -p PORT flash monitor
```

To exit the serial monitor, type ``Ctrl-]``.

See [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/release-v5.3/esp32/index.html) for full steps to configure and build an ESP-IDF project.


