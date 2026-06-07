#ifndef _AHT10_H_
#define _AHT10_H_

#include "driver/i2c_master.h"

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
    float temperature;
    float humidity;
} aht10_dev_t;

// 初始化 AHT10 (传入已配置好的总线句柄)
esp_err_t aht10_init(i2c_master_bus_handle_t bus_handle, aht10_dev_t *sensor);

// 读取温湿度
esp_err_t aht10_read_measurements(aht10_dev_t *sensor);

#endif