#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/sensor.h>

#include "WS2812Led.h"
#include "RtosUtils.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define DISPLAY_DRIVER   DT_CHOSEN(zephyr_display)
#define TEMP_HUM_SENSOR  DT_ALIAS(temphum0)
#define STRIP_NODE       DT_ALIAS(led_strip)

static const struct device *const display_dev = DEVICE_DT_GET(DISPLAY_DRIVER);
static const struct device *const temp_hum_dev = DEVICE_DT_GET(TEMP_HUM_SENSOR);
static const struct device *const rgbled_dev = DEVICE_DT_GET(STRIP_NODE);

#define FONT_INDEX  4

static struct params {
    uint16_t rows;
    uint16_t cols;
    uint16_t x_res;
    uint16_t y_res;
    uint8_t ppt;
    uint8_t font_width;
    uint8_t font_height;
} params;

static WS2812Led led;

static int
init_sensor(const struct device *dev)
{
    if (!device_is_ready(dev))
    {
        LOG_ERR("Temp sensor is not ready.");
        return -1;
    }
    return 0;
}

static void
get_temp_hum(
    const struct device *dev,
    struct sensor_value *temp,
    struct sensor_value *hum)
{
    if (sensor_sample_fetch(dev))
    {
        LOG_ERR("Failed to fetch sensor data.");
        return;
    }

    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, temp);
    sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, hum);
}

static void
update_display(
    const struct device *display,
    const struct sensor_value *temp,
    const struct sensor_value *hum)
{
    double deg_c = temp->val1 + (double)(temp->val2)*0.000001;
    double deg_f = deg_c*9/5 + 32.;
    double hum_pct = hum->val1 + (double)(hum->val2)*0.000001;
    char str[12];

    LOG_DBG("Temperature: %d %d", temp->val1, temp->val2);
    LOG_DBG("Humidity   : %d %d", hum->val1, hum->val2);

    snprintf(str, sizeof(str), "Tmp: %d", (unsigned int)(deg_f + 0.5));
    cfb_print(display, str, 0, 0);
    snprintf(str, sizeof(str), "Hum: %u", hum->val1);
    cfb_print(display, str, 0, 8);
    cfb_framebuffer_finalize(display_dev);
}

static int
init_display(const struct device *dev, struct params *p)
{
    if (!device_is_ready(dev))
    {
        return -ENODEV;
    }

    if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0)
    {
        LOG_ERR("Failed to set required pixel format");
        return -1;
    }

    if (cfb_framebuffer_init(dev))
    {
        LOG_ERR("Framebuffer initialization failed!");
        return -1;
    }

    cfb_framebuffer_clear(dev, true);

    display_blanking_off(dev);

    p->rows  = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
    p->cols  = cfb_get_display_parameter(dev, CFB_DISPLAY_COLS);
    p->x_res = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);
    p->y_res = cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH);
    p->ppt   = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

    int num_fonts = cfb_get_numof_fonts(dev);

    for (int idx = 0; idx < num_fonts; idx++)
    {
        uint8_t h, w;
        cfb_get_font_size(dev, idx, &w, &h);
        LOG_INF("Index[%d] font dimensions %2dx%d", idx, w, h);
    }

    cfb_framebuffer_set_font(dev, FONT_INDEX);
    cfb_get_font_size(dev, FONT_INDEX, &p->font_width, &p->font_height);

    LOG_INF("Selected font: index[%d]", FONT_INDEX);

    LOG_INF("x_res %d, y_res %d, ppt %d, rows %d, cols %d",
        p->x_res,
        p->y_res,
        p->ppt,
        p->rows,
        p->cols);

    return 0;
}


int main(void)
{
    int sensor_ready;
    struct sensor_value temp;
    struct sensor_value hum;
    WS2812Led_Segment *led_seg = &led.seg;

    sensor_ready = init_sensor(temp_hum_dev);

    init_display(display_dev, &params);

    WS2812LED_INIT_SIMPLE(rgbled_dev, &led, "led", 1);

    /* Start blend around the color wheel. */
    CHSV color1 = WS2812LED_HSV_COLOR(HUE_GREEN, 255, 100);
    CHSV color2 = WS2812LED_HSV_COLOR(color1.h + 255, 255, 100);
    led_seg->show(led_seg);
    led_seg->blend(led_seg, true, &color1, &color2, GRAD_LONGEST, 200, 50);

    while (1)
    {
        RTOS_TASK_SLEEP_ms(1000);

        if (sensor_ready == 0)
        {
            get_temp_hum(temp_hum_dev, &temp, &hum);
            update_display(display_dev, &temp, &hum);
        }
    }
}
