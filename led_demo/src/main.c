/** @brief LED + eth RPC demo
*/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

int
main(void)
{
    int ret;

    LOG_INF("led_demo starting.");

    /* Not fatal: a board without an led0 alias logs a warning and continues. */
    (void)app_led_init();

    ret = app_net_init();
    if (ret < 0)
    {
        LOG_ERR("Network init failed: %d", ret);
        return 0;
    }

    ret = app_rpc_init();
    if (ret < 0)
    {
        LOG_ERR("RPC init failed: %d", ret);
        return 0;
    }

    app_trace_init();

    while (1)
    {
        k_msleep(5000);
    }

    return 0;
}
