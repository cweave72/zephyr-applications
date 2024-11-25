#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>

#include "WS2812Led.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define STRIP_NODE              DT_ALIAS(led_strip)

#ifdef CONFIG_LED_POWER
static const struct gpio_dt_spec ledpwr = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS        DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define NUM_PIXELS  DT_PROP(STRIP_NODE, chain_length)
static const struct device *const dev = DEVICE_DT_GET(STRIP_NODE);

WS2812Led_Strip led_strip = {
    .numPixels = NUM_PIXELS,
    .loopDelay_ms = 10,
    .taskStackSize = 4*1024,
    .taskName = "Led0",
    .taskPrio = 15
};

WS2812Led_Segment segment0 = {
    .startIdx = 0,
    .endIdx = NUM_PIXELS-1,
    .taskName = "seg0",
    .taskStackSize = 2*1024,
    .taskPrio = 10,
    .loopDelay_ms = 10
};

int main(void)
{
    int ret;

    LOG_INF("Starting ws2812 demo app.");

#ifdef CONFIG_LED_POWER
    LOG_INF("Applying Led power GPIO.");
    /* Provide power to RGB led. */
    if (!gpio_is_ready_dt(&ledpwr))
    {
        LOG_ERR("GPIO device is not ready.");        
    }
    /* Enable RGB LED */
    gpio_pin_configure_dt(&ledpwr, GPIO_OUTPUT_ACTIVE);
#endif

    /* Use core 1 to mitigate preemption from wifi task. */
    ret = WS2812Led_init(dev, &led_strip, 1);

    WS2812Led_addSegment(&led_strip, &segment0);

    segment0.off(&segment0);
    WS2812Led_show(&led_strip);

    while (1)
    {
        int i, j;
        CHSV start, end, color;

#if 0
        LOG_INF("Fire");
        segment0.fire(&segment0, true, 120, 100, 10);
        RTOS_TASK_SLEEP_s(20);
#endif

#if 1
        LOG_INF("Dissolve");
        color = WS2812LED_COLOR(HUE_AQUA, 255, 140);
        segment0.dissolve(&segment0, true, &color, 80, 20, 50);
        RTOS_TASK_SLEEP_s(20);
#endif

#if 0
        LOG_INF("Meteor");
        color = WS2812LED_COLOR(HUE_BLUE, 255, 240);
        segment0.meteor(&segment0, true, &color, 1, 80, true, 20);
        RTOS_TASK_SLEEP_s(20);
#endif

#if 1
        LOG_INF("Sparkle");
        color = WS2812LED_COLOR(HUE_BLUE, 255, 140);
        segment0.sparkle(&segment0, true, &color, 1, 10);
        RTOS_TASK_SLEEP_s(10);
#endif

#if 0
        LOG_INF("Twinkle");
        segment0.twinkle(&segment0, true, 50, 10);
        RTOS_TASK_SLEEP_s(10);
#endif

#if 1
        LOG_INF("Fill random");
        for (i = 0; i < 100; i++)
        {
            segment0.fill_random(&segment0, 255, 50);
            RTOS_TASK_SLEEP_ms(100);
        }
#endif

#if 1
        LOG_INF("Fill rainbow");
        for (i = 0; i < 1000; i++)
        {
            segment0.fill_rainbow(&segment0, i, 255, 70);
            RTOS_TASK_SLEEP_ms(10);
        }
#endif

#if 0
        LOG_INF("Fill solid");
        for (j = 0; j < 255; j+=10)
        {
            for (i = 0; i < 200; i++)
            {
                uint8_t v;
                if (i < 100)
                {
                    v = i;
                }
                else
                {
                    v = 200 - i;
                }

                start = (CHSV){ .h = j, .s = 255, .v = v };
                segment0.fill_solid(&segment0, &start);
                RTOS_TASK_SLEEP_ms(10);
            }
        }

#endif

#if 1
        LOG_INF("Blend");
        start = (CHSV){ .h = HUE_BLUE, .s = 255, .v = 80 };
        end = (CHSV){ .h = HUE_BLUE-1, .s = 255, .v = 80 };
        segment0.blend(&segment0,
                       true,
                       &start,
                       &end,
                       GRAD_LONGEST,
                       800,
                       20);

        RTOS_TASK_SLEEP_s(15);
#endif
    }

}
