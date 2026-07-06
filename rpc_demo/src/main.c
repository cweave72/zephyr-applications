/** @brief This examples demonstrates: RPC over Wifi
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include "SwTimer.h"
#include "rpc.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/******************************************************************************/
#if defined(CONFIG_APP_NET_TYPE_WIFI)
#include "NvParms.h"
#include "WifiConnect.h"

static void
wifi_error(void)
{
    LOG_ERR("Rebooting system.");
    sys_reboot(SYS_REBOOT_COLD);
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

    //WifiConnect_init();
    WifiConnect_connect(ssid, pass, wifi_error);
    return 0;
}
#endif

#if defined(CONFIG_APP_NET_TYPE_SERIAL)
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
                    NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback mgmt_cb;
static bool connected = false;

static K_SEM_DEFINE(l4_connected, 0, 1);

static void
init_ip(void)
{
    struct net_if *iface;
    struct net_if_addr *ifaddr;
    struct in_addr my_ipv4_addr;
    struct in_addr my_ipv4_mask;
    struct in_addr my_ipv4_gw;

    iface = net_if_get_default();

    LOG_INF("Setting IP address for iface:");
    LOG_INF("  addr: %s", CONFIG_APP_IPV4_ADDR);
    LOG_INF("  mask: %s", CONFIG_APP_IPV4_MASK);
    LOG_INF("  gw  : %s", CONFIG_APP_IPV4_GW);

    net_addr_pton(AF_INET, CONFIG_APP_IPV4_ADDR, &my_ipv4_addr);
    net_addr_pton(AF_INET, CONFIG_APP_IPV4_MASK, &my_ipv4_mask);
    net_addr_pton(AF_INET, CONFIG_APP_IPV4_GW, &my_ipv4_gw);

    ifaddr = net_if_ipv4_addr_add(iface, &my_ipv4_addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr)
    {
        LOG_ERR("Error setting IP address");
        return;
    }

    net_if_ipv4_set_netmask_by_addr(iface, &my_ipv4_addr, &my_ipv4_mask);
    net_if_ipv4_set_gw(iface, &my_ipv4_gw);
}

static void
l4_event_handler(
    struct net_mgmt_event_callback *cb,
    uint32_t mgmt_event,
    struct net_if *iface)
{
    ARG_UNUSED(iface);
    ARG_UNUSED(cb);

    LOG_DBG("mgmt_event = 0x%08x (EVENT_MASK = 0x%08x).",
        mgmt_event, (unsigned int)EVENT_MASK);

    switch (mgmt_event)
    {
    case NET_EVENT_L4_CONNECTED:
        LOG_DBG("NET_EVENT_L4_CONNECTED");
        connected = true;
        k_sem_give(&l4_connected);
        break;
    case NET_EVENT_L4_DISCONNECTED:
        LOG_DBG("NET_EVENT_L4_DISCONNECTED");
        if (connected)
        {
            LOG_INF("Network disconnected event");
            connected = false;
        }
        LOG_DBG("Resetting sem.");
        k_sem_reset(&l4_connected);
        break;
    case NET_EVENT_L4_IPV4_CONNECTED:
        LOG_DBG("NET_EVENT_L4_IPV4_CONNECTED");
        break;
    case NET_EVENT_L4_IPV4_DISCONNECTED:
        LOG_DBG("NET_EVENT_L4_IPV4_DISCONNECTED");
        break;
    case NET_EVENT_L4_IPV6_CONNECTED:
        LOG_DBG("NET_EVENT_L4_IPV6_CONNECTED");
        break;
    case NET_EVENT_L4_IPV6_DISCONNECTED:
        LOG_DBG("NET_EVENT_L4_IPV6_DISCONNECTED");
        break;

    default:
        break;
    }

}

static int
network_init(void)
{
    int ret;

    if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER))
    {
        LOG_ERR("Must set CONFIG_NET_CONNECTION_MANAGER=y in Kconfig.");
        return -1;
    }

    net_mgmt_init_event_callback(&mgmt_cb, l4_event_handler, EVENT_MASK);
    net_mgmt_add_event_callback(&mgmt_cb);
    conn_mgr_mon_resend_status();

    init_ip();

    ret = net_config_init_app(NULL, "Initializing network");
    if (ret < 0)
    {
        LOG_ERR("Failed network init (%d)", ret);
        return ret;
    }

    LOG_INF("Waiting for network connection...");

    /* Wait for the connection. */
    k_sem_take(&l4_connected, K_FOREVER);

    LOG_INF("Network connected.");

    return 0;
}
#endif


#if defined(CONFIG_TRACERAM)
#include <ctf_top.h>
#include "TraceRam.h"
//
//static inline void user_0(uint32_t el)
//{
//    CTF_EVENT(CTF_LITERAL(uint8_t, 0x99), el);
//}
#endif

int main(void)
{
    int ret;
    SwTimer t;

    LOG_INF("RPC demo app.");

#if defined(CONFIG_APP_NET_TYPE_WIFI)
    ret = NvParms_init();
    if (ret < 0)
    {
        LOG_ERR("NvParms module init error : %d", ret);
        return 0;
    }
    init_wifi();
#endif

#if defined(CONFIG_APP_NET_TYPE_SERIAL)
    network_init();
#endif

    rpc_init();
    rpc_start_server();

#if defined(CONFIG_TRACERAM)
    LOG_INF("Enabling trace ram.");
    TraceRam_enable();
#endif

    while (1)
    {

        k_msleep(40000);
        // Force disconnect for testing.
        //struct net_if *iface = net_if_get_default();
        //net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

        //#if defined(CONFIG_TRACERAM)
        //        user_0(el);
        //#endif

    }

    return 0;
}
