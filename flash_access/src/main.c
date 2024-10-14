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
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/settings/settings.h>

#include <spi_flash_mmap.h>
#include <soc.h>
#include <errno.h>

#include "SwTimer.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/** @brief Demonstration thread for blinking LED. */
static void led_flash(void *arg0, void *arg1, void *arg2)
{
    int led_state = 1;
    int ret;

    ARG_UNUSED(arg0);
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

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
        gpio_pin_set_dt(&led, led_state);
        led_state = !led_state;
        k_msleep(500);
    }
}

K_THREAD_DEFINE(led_thread, 1024, led_flash, NULL, NULL, NULL, 10, 0, 0);

#define FLASH_PARTITION scratch_partition

static void timer_cb(struct k_timer *t)
{
    static unsigned int count = 0;
    printk("Hello %u\n", count++);
}

static SwTimer swt = {
    .expire_cb = timer_cb,
    .stop_cb = NULL,
    .type = SWTIMER_TYPE_ONE_SHOT
};

int main(void)
{
    uint8_t buffer[32];
    const struct device *flash_device;
    const void *mem_ptr;
    spi_flash_mmap_handle_t handle;
    off_t address = FIXED_PARTITION_OFFSET(FLASH_PARTITION);
    size_t size = FIXED_PARTITION_SIZE(FLASH_PARTITION);
    int rc;

    LOG_INF("Starting flash_access app.");

    flash_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    if (!device_is_ready(flash_device))
    {
        LOG_ERR("%s: device not ready.", flash_device->name);
        return 0;
    }

    printf("address = 0x%08x\n", address);
    printf("size = 0x%08x\n", size);

#if 1
    /* map selected region */
    spi_flash_mmap(address, size, SPI_FLASH_MMAP_DATA, &mem_ptr, &handle);
    LOG_INF("memory-mapped pointer address: %p", mem_ptr);
    LOG_HEXDUMP_INF(mem_ptr, 32, "flash read using memory-mapped pointer");
#endif

    rc = settings_subsys_init();
    if (rc)
    {
        LOG_ERR("Settings error : %d", rc);
    }

    SwTimer_create(&swt);
    SwTimer_start_ms(&swt, 5000);

    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
