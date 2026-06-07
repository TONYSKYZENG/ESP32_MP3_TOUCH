#include "aht10.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AHT10_I2C_ADDR 0x38

esp_err_t aht10_init(i2c_master_bus_handle_t bus_handle, aht10_dev_t *sensor) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT10_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    
    // 将设备挂载到 I2C 总线上
    return i2c_master_bus_add_device(bus_handle, &dev_cfg, &sensor->i2c_dev);
}

esp_err_t aht10_read_measurements(aht10_dev_t *sensor) {
    uint8_t trigger_cmd[] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    // 发送触发命令
    esp_err_t ret = i2c_master_transmit(sensor->i2c_dev, trigger_cmd, sizeof(trigger_cmd), -1);
    if (ret != ESP_OK) return ret;

    // 必须等待转换完成 (AHT10 典型值 75ms)
    vTaskDelay(pdMS_TO_TICKS(80));

    // 读取 6 字节
    ret = i2c_master_receive(sensor->i2c_dev, data, sizeof(data), -1);
    if (ret != ESP_OK) return ret;

    // 数据转换
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    sensor->humidity = (float)hum_raw / 1048576.0 * 100.0;
    sensor->temperature = (float)temp_raw / 1048576.0 * 200.0 - 50.0;

    return ESP_OK;
}