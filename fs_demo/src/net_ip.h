/*******************************************************************************
 *  @file: net_ip.h
 *
 *  @brief: IPv4 addressing from /flash/etc/config/net.pb, with the build-time
 *  CONFIG_APP_IPV4_* values as the fallback. Compiled only under
 *  CONFIG_APP_USE_STATIC_IP.
*******************************************************************************/
#ifndef APP_NET_IP_H
#define APP_NET_IP_H

/******************************************************************************
    [docexport app_net_static_ip_apply]
*//**
    @brief Applies the IPv4 settings of net.pb to the default interface. A
    missing file or field uses CONFIG_APP_IPV4_ADDR/MASK/GW.
******************************************************************************/
void
app_net_static_ip_apply(void);
#endif
