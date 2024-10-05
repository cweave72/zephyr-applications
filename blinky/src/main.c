/** @brief This examples demonstrates:
    Using the devicetree for accessing a gpio led.
    Creating threads.
    Using event flags.
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define SLEEP_TIME_MS   100

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_EVENT_DEFINE(event_flags);
#define FLAG_LED_TOGGLE     0x00000001

#define THREAD_STACKSIZE 1024
#define THREAD_PRIO  7

K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACKSIZE);
static struct k_thread thread;

/** @brief Demonstration thread for blinking LED. */
static void thread_entry(void *arg0, void *arg1, void *arg2)
{
    int led_state = 1;
    int ret;

    ARG_UNUSED(arg0);
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    LOG_INF("Starting thread.");

    if (!gpio_is_ready_dt(&led))
    {
        LOG_ERR("GPIO device is not ready.");        
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("gpio_pin_configure returned %d", ret);
        return;
    }

    while (1)
    {
        uint32_t flags;

        /* Wait on the LED TOGGLE flag */
        flags = k_event_wait(
            &event_flags,
            FLAG_LED_TOGGLE,    // which flag to wait on
            false,              // true=reset on detect; false=don't reset
            K_FOREVER);         // timeout

        if (flags & FLAG_LED_TOGGLE)
        {
            /* Clear because event_wait is not auto-clearing. */
            k_event_clear(&event_flags, FLAG_LED_TOGGLE);
            gpio_pin_set_dt(&led, led_state);
            led_state = !led_state;
        }
    }
}

int main(void)
{
    LOG_INF("Starting blinky app.");

    k_thread_create(
        &thread,
        thread_stack,
        K_THREAD_STACK_SIZEOF(thread_stack),
        thread_entry,
        NULL, NULL, NULL,
        THREAD_PRIO,
        0,
        K_FOREVER);
    k_thread_name_set(&thread, "led thread");
    k_thread_start(&thread);

    while (1)
    {
        k_event_set(&event_flags, FLAG_LED_TOGGLE);
        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
