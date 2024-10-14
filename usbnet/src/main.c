#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>

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

static int
init_usb(void)
{
    int ret;

    //ret = usb_enable(NULL);
    //LOG_DBG("here1");
    //if (ret != 0)
    //{
    //    LOG_ERR("Cannot enable USB (%d)", ret);
    //    return ret;
    //}

    ret = net_config_init_app(NULL, "Initializing network");
    if (ret < 0)
    {
        LOG_ERR("Failed config init USB (%d)", ret);
        return ret;
    }
    return 0;
}

static void
event_handler(
    struct net_mgmt_event_callback *cb,
    uint32_t mgmt_event,
    struct net_if *iface)
{
    ARG_UNUSED(iface);
    ARG_UNUSED(cb);

    LOG_DBG("event = 0x%08x.", mgmt_event);

    if ((mgmt_event & EVENT_MASK) != mgmt_event)
    {
        LOG_DBG("Not an event of interest, ignoring.");
        return;
    }

    if (mgmt_event == NET_EVENT_L4_CONNECTED)
    {
        LOG_INF("Network connected event");

        connected = true;
        k_sem_give(&run_app);

        return;
    }

    if (mgmt_event == NET_EVENT_L4_DISCONNECTED)
    {
        if (connected == false)
        {
            LOG_INF("Network Disconnected event. Waiting network to be connected");
        }
        else
        {
            LOG_INF("Network disconnected event");
            connected = false;
        }

        LOG_DBG("Resetting sem.");
        k_sem_reset(&run_app);

        return;
    }
}

static void
init_app(void)
{
    LOG_DBG("In init app.");
    if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER))
    {
        net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
        net_mgmt_add_event_callback(&mgmt_cb);
        conn_mgr_mon_resend_status();
    }

    LOG_DBG("Initializing usb.");
    init_usb();
}

int main(void)
{
    int ret;

    LOG_INF("usbnet app.");

    init_app();

    LOG_DBG("Waiting on sem.");
    /* Wait for the connection. */
    k_sem_take(&run_app, K_FOREVER);
    LOG_DBG("Took sem.");

    while (1)
    {
        if (connected)
        {
            LOG_INF("Connected");
        }
        else
        {
            LOG_INF("Not connected");
            
        }
        k_msleep(1000);
    }

    return 0;
}
