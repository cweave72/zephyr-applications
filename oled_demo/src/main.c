#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>

#include "RtosUtils.h"
#include "SwTimer.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define DISPLAY_DRIVER   DT_CHOSEN(zephyr_display)
static const struct device *const dev = DEVICE_DT_GET(DISPLAY_DRIVER);

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
    SwTimer timer;
    uint32_t k = 0;
    uint32_t delta = 0;
    init_display(dev, &params);

    while (1)
    {
        char str[9];

        SwTimer_tic(&timer);
        snprintf(str, sizeof(str), "%u", k++);
        cfb_print(dev, str, 0, 0);
        snprintf(str, sizeof(str), "%u us", delta);
        cfb_print(dev, str, 0, 8);
        cfb_framebuffer_finalize(dev);
        delta = SwTimer_toc(&timer);

        RTOS_TASK_SLEEP_ms(100);
    }
}
