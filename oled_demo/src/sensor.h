/*******************************************************************************
 *  @file: sensor.h
 *   
 *  @brief: Header for sensor functions
*******************************************************************************/
#ifndef SENSOR_H
#define SENSOR_H

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/sensor.h>

int init_sensor(void);
void get_temp_hum(struct sensor_value *temp, struct sensor_value *hum);
int init_display(void);
void update_display(const struct sensor_value *temp, const struct sensor_value *hum);

int
encode_json_result(
    const struct sensor_value *temp,
    const struct sensor_value *hum,
    char *buffer,
    uint32_t max_size);

void sensor_conv_to_fixedpt(
    const struct sensor_value *temp,
    const struct sensor_value *hum,
    uint8_t temp_fl,
    uint8_t hum_fl,
    int32_t *deg_f,
    int32_t *hum_pct);
#endif
