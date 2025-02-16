/*******************************************************************************
 *  @file: sensor.c
 *  
 *  @brief: Code for reading sensor data
*******************************************************************************/
#include <zephyr/data/json.h>
#include "lwm2m_util.h"
#include "sensor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app, LOG_LEVEL_INF);

#define DISPLAY_DRIVER   DT_CHOSEN(zephyr_display)
#define TEMP_HUM_SENSOR  DT_ALIAS(temphum0)

static const struct device *const display_dev = DEVICE_DT_GET(DISPLAY_DRIVER);
static const struct device *const temp_hum_dev = DEVICE_DT_GET(TEMP_HUM_SENSOR);

static struct dparams {
    uint16_t rows;
    uint16_t cols;
    uint16_t x_res;
    uint16_t y_res;
    uint8_t ppt;
    uint8_t font_width;
    uint8_t font_height;
} dparams;

#define FONT_INDEX  4

struct sensor_result {
    struct json_obj_token temp;
    struct json_obj_token hum;
};

static const struct json_obj_descr json_result[] = {
    JSON_OBJ_DESCR_PRIM(struct sensor_result, temp, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(struct sensor_result, hum, JSON_TOK_FLOAT),
};

static int
float_to_string(double *value, char *out, uint32_t outlen, uint8_t num_precision)
{
    int len;

    len = lwm2m_ftoa(value, out, outlen, num_precision);
    if (len < 0 || len >= outlen)
    {
        LOG_ERR("Failed to encode float value");
        return -EINVAL;
    }

    return len;
}

int
init_sensor(void)
{
    if (!device_is_ready(temp_hum_dev))
    {
        LOG_ERR("Temp sensor is not ready.");
        return -1;
    }
    return 0;
}

void
get_temp_hum(struct sensor_value *temp, struct sensor_value *hum)
{
    if (sensor_sample_fetch(temp_hum_dev))
    {
        LOG_ERR("Failed to fetch sensor data.");
        return;
    }

    sensor_channel_get(temp_hum_dev, SENSOR_CHAN_AMBIENT_TEMP, temp);
    sensor_channel_get(temp_hum_dev, SENSOR_CHAN_HUMIDITY, hum);
}

void
update_display(const struct sensor_value *temp, const struct sensor_value *hum)
{
    double deg_c = temp->val1 + (double)(temp->val2)*0.000001;
    double deg_f = deg_c*9/5 + 32.;
    char str[12];

    LOG_DBG("Temperature: %d %d", temp->val1, temp->val2);
    LOG_DBG("Humidity   : %d %d", hum->val1, hum->val2);

    snprintf(str, sizeof(str), "Tmp: %d", (unsigned int)(deg_f + 0.5));
    cfb_print(display_dev, str, 0, 0);
    snprintf(str, sizeof(str), "Hum: %u", hum->val1);
    cfb_print(display_dev, str, 0, 8);
    cfb_framebuffer_finalize(display_dev);
}

int
encode_json_result(
    const struct sensor_value *temp,
    const struct sensor_value *hum,
    char *buffer,
    uint32_t max_size)
{
    int ret;
    double deg_c = temp->val1 + (double)(temp->val2)*0.000001;
    double deg_f = deg_c*9/5 + 32.;
    double hum_pct = hum->val1 + (double)(hum->val2)*0.000001;
    char temp_fl[24];
    char hum_fl[24];
    int fllen;

    struct sensor_result rs;

    fllen = float_to_string(&deg_f, temp_fl, sizeof(temp_fl), 2);
    if (fllen < 0)
    {
        return -1;
    }
    rs.temp.start = temp_fl;
    rs.temp.length = fllen;

    fllen = float_to_string(&hum_pct, hum_fl, sizeof(hum_fl), 2);
    if (fllen < 0)
    {
        return -1;
    }
    rs.hum.start = hum_fl;
    rs.hum.length = fllen;

    int len = json_calc_encoded_len(
        json_result, ARRAY_SIZE(json_result), &rs);

    if (len < 0)
    {
        LOG_ERR("Error json_calc_encoded_len: %d", len);
        return (int)len;
    }

    if (len > max_size)
    {
        LOG_ERR("Buffer not large enough to encode json (%u > %u).",
            len, max_size);
        return -EINVAL;
    }

    ret = json_obj_encode_buf(json_result, ARRAY_SIZE(json_result),
        &rs, buffer, max_size);
    if (ret < 0)
    {
        LOG_ERR("Json encode error: %d", ret);
        return ret;
    }

    return len;
}

int
init_display(void)
{
    if (!device_is_ready(display_dev))
    {
        return -ENODEV;
    }

    if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10) != 0)
    {
        LOG_ERR("Failed to set required pixel format");
        return -1;
    }

    if (cfb_framebuffer_init(display_dev))
    {
        LOG_ERR("Framebuffer initialization failed!");
        return -1;
    }

    cfb_framebuffer_clear(display_dev, true);

    display_blanking_off(display_dev);

    dparams.rows  = cfb_get_display_parameter(display_dev, CFB_DISPLAY_ROWS);
    dparams.cols  = cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS);
    dparams.x_res = cfb_get_display_parameter(display_dev, CFB_DISPLAY_WIDTH);
    dparams.y_res = cfb_get_display_parameter(display_dev, CFB_DISPLAY_HEIGH);
    dparams.ppt   = cfb_get_display_parameter(display_dev, CFB_DISPLAY_PPT);

    int num_fonts = cfb_get_numof_fonts(display_dev);

    for (int idx = 0; idx < num_fonts; idx++)
    {
        uint8_t h, w;
        cfb_get_font_size(display_dev, idx, &w, &h);
        LOG_INF("Index[%d] font dimensions %2dx%d", idx, w, h);
    }

    cfb_framebuffer_set_font(display_dev, FONT_INDEX);
    cfb_get_font_size(
        display_dev,
        FONT_INDEX,
        &dparams.font_width,
        &dparams.font_height);

    LOG_INF("Selected font: index[%d]", FONT_INDEX);

    LOG_INF("x_res %d, y_res %d, ppt %d, rows %d, cols %d",
        dparams.x_res,
        dparams.y_res,
        dparams.ppt,
        dparams.rows,
        dparams.cols);

    return 0;
}
