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
#include "WifiConnect.h"
#include "TcpEcho.h"
#include "NvParms.h"

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

static TcpEcho tcp_echo;

static int
setup_gpio(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        LOG_ERR("GPIO device is not ready.");        
        return -1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("gpio_pin_configure returned %d", ret);
        return -1;
    }
    return 0;
}

static int
init_wifi(void)
{
    char ssid[32];
    char pass[32];
    int ret, pass_len;

    ret = NvParms_load("ssid", NVPARMS_TYPE_STRING, ssid, sizeof(ssid));
    if (ret <= 0)
    {
        LOG_ERR("Error getting ssid from NV: %d", ret);
        return -1;
    }

    pass_len = NvParms_load("pass", NVPARMS_TYPE_STRING, pass, sizeof(pass));
    if (pass_len <= 0)
    {
        LOG_ERR("Error getting pass from NV: %d", pass_len);
        return -1;
    }

    LOG_DBG("ssid=%s", ssid);
    LOG_DBG("password length=%d", pass_len);

    WifiConnect_init();
    WifiConnect_connect(ssid, pass);
    return 0;
}

/** @brief Demonstration thread for blinking LED. */
static void thread_entry(void *arg0, void *arg1, void *arg2)
{
    int led_state = 1;
    int ret;

    ARG_UNUSED(arg0);
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    LOG_INF("Starting thread.");

    ret = setup_gpio();
    if (ret < 0)
    {
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

#define LED_UP_PERIOD_MS    200
#define LED_DN_PERIOD_MS    1000

int main(void)
{
    int led_state = 1;
    int ret;

    LOG_INF("Wifi station app.");

    k_thread_create(
        &thread,
        thread_stack,
        K_THREAD_STACK_SIZEOF(thread_stack),
        thread_entry,
        NULL, NULL, NULL,
        THREAD_PRIO,
        0,
        K_NO_WAIT);
    k_thread_name_set(&thread, "led thread");

    ret = NvParms_init();
    if (ret < 0)
    {
        LOG_ERR("NvParms module init error : %d", ret);
        return 0;
    }

    init_wifi();

    ret = TcpEcho_init(
        &tcp_echo,
        12001,
        NULL,
        1024,
        4*1024,
        "TCP Echo",
        20);
    if (ret < 0) LOG_ERR("Error initializing Tcp Echo server: %d",  ret);

    while (1)
    {
        int state;
        uint32_t sleep;

        state = WifiConnect_getState();

        k_event_set(&event_flags, FLAG_LED_TOGGLE);
        sleep = (state) ? LED_UP_PERIOD_MS : LED_DN_PERIOD_MS;
        k_msleep(sleep);
    }

    return 0;
}
