/*******************************************************************************
 *  @file: net_ip.c
 *
 *  @brief: IPv4 addressing. See net_ip.h.
 *
 *  The values come from the [ipv4] section of /flash/etc/config/net.conf
 *  (written by branding, see the README). A missing file or key falls back to
 *  the build-time value: CONFIG_APP_IPV4_ADDR, _MASK and _GW.
*******************************************************************************/
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#if defined(CONFIG_NET_DHCPV4)
#include <zephyr/net/dhcpv4.h>
#endif

#include "IniFileParser.h"
#include "net_ip.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief The network configuration file. */
#define NET_CONF        "/flash/etc/config/net.conf"
#define NET_CONF_IPV4   "ipv4"

/** @brief Size of a dotted-quad address, with the NUL. */
#define ADDR_LEN        sizeof("255.255.255.255")

/******************************************************************************
    get_addr
*//**
    @brief Reads an IPv4 address from the [ipv4] section of net.conf. Uses the
    build-time default when the key is missing or not a valid address.
    @param[in] key  The key name.
    @param[in] def  The build-time default.
    @param[out] addr  The address.
    @return The source of the value, for the log: "net.conf" or "Kconfig".
******************************************************************************/
static const char *
get_addr(const char *key, const char *def, struct in_addr *addr)
{
    char buf[ADDR_LEN];
    int ret;

    ret = IniFileParser_get(NET_CONF, NET_CONF_IPV4, key, buf, sizeof(buf));
    if (ret > 0)
    {
        if (net_addr_pton(AF_INET, buf, addr) == 0)
        {
            return "net.conf";
        }
        LOG_WRN("%s [%s] %s: '%s' is not an IPv4 address.", NET_CONF,
            NET_CONF_IPV4, key, buf);
    }
    else if (ret != -ENOENT)
    {
        LOG_WRN("%s [%s] %s: read error %d.", NET_CONF, NET_CONF_IPV4, key,
            ret);
    }

    (void)net_addr_pton(AF_INET, def, addr);
    return "Kconfig";
}

/******************************************************************************
    use_dhcp
*//**
    @brief Returns true if net.conf selects DHCP (mode = dhcp) and the build
    has DHCP. Otherwise the address is static.
******************************************************************************/
static bool
use_dhcp(void)
{
    char mode[8];

    if (IniFileParser_get(NET_CONF, NET_CONF_IPV4, "mode", mode,
        sizeof(mode)) <= 0)
    {
        return false;
    }
    if (strcmp(mode, "dhcp") == 0)
    {
        if (IS_ENABLED(CONFIG_NET_DHCPV4))
        {
            return true;
        }
        LOG_WRN("%s: mode = dhcp, but the build has no DHCP "
            "(CONFIG_NET_DHCPV4). Using a static address.", NET_CONF);
    }
    else if (strcmp(mode, "static") != 0)
    {
        LOG_WRN("%s: unknown mode '%s'. Using a static address.", NET_CONF,
            mode);
    }
    return false;
}

/******************************************************************************
    [docimport app_net_static_ip_apply]
*//**
    @brief Applies the IPv4 settings of net.conf to the default interface. A
    missing file or key uses CONFIG_APP_IPV4_ADDR/MASK/GW.
******************************************************************************/
void
app_net_static_ip_apply(void)
{
    struct net_if *iface;
    struct net_if_addr *ifaddr;
    struct in_addr addr;
    struct in_addr mask;
    struct in_addr gw;
    char text[3][ADDR_LEN];
    const char *src[3];

    iface = net_if_get_default();

#if defined(CONFIG_NET_DHCPV4)
    if (use_dhcp())
    {
        LOG_INF("Starting DHCP (%s).", NET_CONF);
        net_dhcpv4_start(iface);
        return;
    }
#else
    (void)use_dhcp();
#endif

    src[0] = get_addr("address", CONFIG_APP_IPV4_ADDR, &addr);
    src[1] = get_addr("netmask", CONFIG_APP_IPV4_MASK, &mask);
    src[2] = get_addr("gateway", CONFIG_APP_IPV4_GW, &gw);

    net_addr_ntop(AF_INET, &addr, text[0], sizeof(text[0]));
    net_addr_ntop(AF_INET, &mask, text[1], sizeof(text[1]));
    net_addr_ntop(AF_INET, &gw, text[2], sizeof(text[2]));

    LOG_INF("Setting static IP address for iface:");
    LOG_INF("  addr: %s (%s)", text[0], src[0]);
    LOG_INF("  mask: %s (%s)", text[1], src[1]);
    LOG_INF("  gw  : %s (%s)", text[2], src[2]);

    ifaddr = net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
    if (!ifaddr)
    {
        LOG_ERR("Error setting IP address");
        return;
    }

    net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
    net_if_ipv4_set_gw(iface, &gw);
}
