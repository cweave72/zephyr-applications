#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/drivers/led_strip.h>

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define RGBLED_NODE         DT_ALIAS(led_strip)
#define RGBLEDPWR_NODE      DT_ALIAS(led0)

static const struct device *const rgb_led = DEVICE_DT_GET(RGBLED_NODE);
static const struct gpio_dt_spec ledpwr = GPIO_DT_SPEC_GET(RGBLEDPWR_NODE, gpios);

#define DELAY_TIME K_MSEC(1000)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb colors[] = {
        RGB(0x0f, 0x00, 0x00), /* red */
        RGB(0x00, 0x0f, 0x00), /* green */
        RGB(0x00, 0x00, 0x0f), /* blue */
};


static struct led_rgb pixels[1];

int main(void)
{
    LOG_INF("Starting qtpy demo app.");

    size_t color = 0;
    int rc;

    /* Provide power to RGB led. */
    if (!gpio_is_ready_dt(&ledpwr))
    {
        LOG_ERR("GPIO device is not ready.");        
    }

    if (device_is_ready(rgb_led))
    {
        LOG_INF("Found LED rgb_led device %s", rgb_led->name);
    } else
    {
        LOG_ERR("LED rgb_led device %s is not ready", rgb_led->name);
        return 0;
    }

    /* Initialize LED */
    memset(&pixels, 0x00, sizeof(pixels));
    memcpy(&pixels[0], &colors[color], sizeof(struct led_rgb));
    rc = led_strip_update_rgb(rgb_led, pixels, 1);

    /* Enable RGB LED */
    rc = gpio_pin_configure_dt(&ledpwr, GPIO_OUTPUT_ACTIVE);

    LOG_INF("Displaying pattern on rgb_led");
    while (1)
    {
        for (size_t cursor = 0; cursor < ARRAY_SIZE(pixels); cursor++)
        {
            memset(&pixels, 0x00, sizeof(pixels));
            memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));

            rc = led_strip_update_rgb(rgb_led, pixels, 1);
            if (rc)
            {
                LOG_ERR("couldn't update rgb_led: %d", rc);
            }

            k_sleep(DELAY_TIME);
        }

        color = (color + 1) % ARRAY_SIZE(colors);
    }

    return 0;
}
