#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);


#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
                    NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback mgmt_cb;
static bool connected = false;

static K_SEM_DEFINE(run_app, 0, 1);

#define IPADDR(o1,o2,o3,o4)   { { { (o1), (o2), (o3), (o4) } } };

static struct in_addr my_ipv4_addr = IPADDR(192, 0, 2, 1);
static struct in_addr my_netmask = IPADDR(255, 255, 255, 0);
static struct in_addr my_gw = IPADDR(192, 0, 2, 2);

static int
init_usb(void)
{
    int ret;

    LOG_DBG("Initializing usb.");

    ret = usb_enable(NULL);
    if (ret != 0)
    {
        LOG_ERR("Cannot enable USB (%d)", ret);
        return ret;
    }

    return 0;
}

static void
init_ip(void)
{
    struct net_if *iface = net_if_get_default();
    struct net_if_addr *ifaddr;

    LOG_DBG("Setting IP address.");
    ifaddr = net_if_ipv4_addr_add(iface, &my_ipv4_addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr)
    {
        LOG_ERR("Error setting IP address");
        return;
    }

    net_if_ipv4_set_netmask(iface, &my_netmask);
    net_if_ipv4_set_gw(iface, &my_gw);
}

static void
event_handler(
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
        k_sem_give(&run_app);
        break;
    case NET_EVENT_L4_DISCONNECTED:
        LOG_DBG("NET_EVENT_L4_DISCONNECTED");
        if (connected)
        {
            LOG_INF("Network disconnected event");
            connected = false;
        }
        LOG_DBG("Resetting sem.");
        k_sem_reset(&run_app);
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
init_app(void)
{
    int ret;

    if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER))
    {
        LOG_DBG("Net Connection Manager enabled.");
        net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
        net_mgmt_add_event_callback(&mgmt_cb);
        conn_mgr_mon_resend_status();
    }

    init_usb();

    init_ip();

    ret = net_config_init_app(NULL, "Initializing network");
    if (ret < 0)
    {
        LOG_ERR("Failed network init (%d)", ret);
        return ret;
    }


    return ret;
}

int main(void)
{
    LOG_INF("usbnet app. %u", CONFIG_NET_MGMT_EVENT_QUEUE_SIZE);

    init_app();

    LOG_DBG("Waiting on connection sem.");

    /* Wait for the connection. */
    k_sem_take(&run_app, K_FOREVER);
    LOG_DBG("Got connection sem.");


    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
