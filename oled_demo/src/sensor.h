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

#endif
