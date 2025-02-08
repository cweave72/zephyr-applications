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
#include "EchoServer.h"
#include "NvParms.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);


static EchoServer echo;

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

int main(void)
{
    int ret;

    LOG_INF("TCP Echo app.");

    ret = NvParms_init();
    if (ret < 0)
    {
        LOG_ERR("NvParms module init error : %d", ret);
        return 0;
    }

    init_wifi();

    ret = EchoServer_init(
        &echo,
        12001,
        NULL,
        1024,
        4*1024,
        "Echo Server",
        20);
    if (ret < 0) LOG_ERR("Error initializing Echo server: %d",  ret);

    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
