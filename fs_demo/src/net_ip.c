/*******************************************************************************
 *  @file: net_ip.c
 *
 *  @brief: IPv4 addressing. See net_ip.h.
 *
 *  The values come from /flash/etc/config/net.pb, a netconf.NetConf protobuf
 *  blob (proto/NetConf.proto). Branding writes it from
 *  brand/<name>/etc/config/net.pb.yaml; fsapi-cli pbput replaces it. A missing
 *  file or empty field falls back to the build-time value:
 *  CONFIG_APP_IPV4_ADDR, _MASK and _GW.
*******************************************************************************/
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#if defined(CONFIG_NET_DHCPV4)
#include <zephyr/net/dhcpv4.h>
#endif

#include "FsApiPb.h"
#include "NetConf.pb.h"
#include "net_ip.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief The network configuration file. */
#define NET_CONF        "/flash/etc/config/net.pb"

/** @brief Size of a dotted-quad address, with the NUL. */
#define ADDR_LEN        sizeof("255.255.255.255")

/******************************************************************************
    load_conf
*//**
    @brief Reads net.pb. A missing or bad file gives an empty configuration,
    so each value uses the build-time default.
    @param[out] conf  The configuration.
******************************************************************************/
static void
load_conf(netconf_NetConf *conf)
{
    int ret;

    ret = FsApi_unpack_file(NET_CONF, conf, netconf_NetConf_fields);
    if (ret == 0)
    {
        return;
    }
    if (ret != -ENOENT)
    {
        LOG_WRN("%s: read error %d. Using the Kconfig values.", NET_CONF, ret);
    }
    /* A failed decode can leave part of the data. */
    *conf = (netconf_NetConf)netconf_NetConf_init_zero;
}

/******************************************************************************
    get_addr
*//**
    @brief Converts an IPv4 address of net.pb. Uses the build-time default
    when the field is empty or not a valid address.
    @param[in] name  The field name, for the log.
    @param[in] text  The field value. An empty string is not set.
    @param[in] def  The build-time default.
    @param[out] addr  The address.
    @return The source of the value, for the log: "net.pb" or "Kconfig".
******************************************************************************/
static const char *
get_addr(const char *name, const char *text, const char *def,
    struct in_addr *addr)
{
    if (text[0] != '\0')
    {
        if (net_addr_pton(AF_INET, text, addr) == 0)
        {
            return "net.pb";
        }
        LOG_WRN("%s ipv4.%s: '%s' is not an IPv4 address.", NET_CONF, name,
            text);
    }

    (void)net_addr_pton(AF_INET, def, addr);
    return "Kconfig";
}

/******************************************************************************
    use_dhcp
*//**
    @brief Returns true if net.pb selects DHCP (mode = IPV4_MODE_DHCP) and the
    build has DHCP. Otherwise the address is static.
    @param[in] ipv4  The ipv4 settings of net.pb.
******************************************************************************/
static bool
use_dhcp(const netconf_Ipv4 *ipv4)
{
    if (ipv4->mode != netconf_Ipv4Mode_IPV4_MODE_DHCP)
    {
        return false;
    }
    if (IS_ENABLED(CONFIG_NET_DHCPV4))
    {
        return true;
    }
    LOG_WRN("%s: mode = IPV4_MODE_DHCP, but the build has no DHCP "
        "(CONFIG_NET_DHCPV4). Using a static address.", NET_CONF);
    return false;
}

/******************************************************************************
    [docimport app_net_static_ip_apply]
*//**
    @brief Applies the IPv4 settings of net.pb to the default interface. A
    missing file or field uses CONFIG_APP_IPV4_ADDR/MASK/GW.
******************************************************************************/
void
app_net_static_ip_apply(void)
{
    static netconf_NetConf conf;
    struct net_if *iface;
    struct net_if_addr *ifaddr;
    struct in_addr addr;
    struct in_addr mask;
    struct in_addr gw;
    char text[3][ADDR_LEN];
    const char *src[3];

    iface = net_if_get_default();
    load_conf(&conf);

#if defined(CONFIG_NET_DHCPV4)
    if (use_dhcp(&conf.ipv4))
    {
        LOG_INF("Starting DHCP (%s).", NET_CONF);
        net_dhcpv4_start(iface);
        return;
    }
#else
    (void)use_dhcp(&conf.ipv4);
#endif

    src[0] = get_addr("address", conf.ipv4.address, CONFIG_APP_IPV4_ADDR,
        &addr);
    src[1] = get_addr("netmask", conf.ipv4.netmask, CONFIG_APP_IPV4_MASK,
        &mask);
    src[2] = get_addr("gateway", conf.ipv4.gateway, CONFIG_APP_IPV4_GW, &gw);

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
