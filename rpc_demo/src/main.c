/** @brief This examples demonstrates: RPC over Wifi
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "WifiConnect.h"
#include "TcpRpcServer.h"
#include "NvParms.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/******************************************************************************/
/** @brief RPC Declarations */
#include "TestRpc.h"
#include "TestRpc.pb.h"
#include "SystemRpc.h"
#include "SystemRpc.pb.h"

#define RPCSERVER_STACK_SIZE    4*1024

static ProtoRpc_Callset_Entry callsets[] = {
    PROTORPC_ADD_CALLSET(1, TestRpc_resolver, test_TestCallset_fields, test_TestCallset_size),
    PROTORPC_ADD_CALLSET(2, SystemRpc_resolver, system_SystemCallset_fields, system_SystemCallset_size),
};

static TcpRpcServer tcp_rpc;
static uint8_t *rpc_call_frame;
static uint8_t *rpc_reply_frame;
static uint8_t *rpc_callset_call_buf;
static uint8_t *rpc_callset_reply_buf;
static ProtoRpc rpc;
/******************************************************************************/

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

/** @brief Allocate frame buffers based on max length callset size and init the
      rpc object. */
static int
rpc_init(void)
{
    uint32_t k;
    uint32_t max_callset_size = 0;

    for (k = 0; k < PROTORPC_ARRAY_LENGTH(callsets); k++)
    {
        if (callsets[k].size > max_callset_size)
        {
            max_callset_size = callsets[k].size;
        }
    }

    LOG_DBG("Max rpc callset size: %u", max_callset_size);

    rpc_call_frame = k_malloc(ProtoRpcHeader_size + max_callset_size);
    if (!rpc_call_frame)
    {
        LOG_ERR("Error allocating memory for rpc_call_frame.");
        return -ENOMEM;
    }

    rpc_reply_frame = k_malloc(ProtoRpcHeader_size + max_callset_size);
    if (!rpc_reply_frame)
    {
        LOG_ERR("Error allocating memory for rpc_reply_frame.");
        return -ENOMEM;
    }

    rpc_callset_call_buf = k_malloc(max_callset_size);
    if (!rpc_callset_call_buf)
    {
        LOG_ERR("Error allocating memory for rpc_callset_call_buf.");
        return -ENOMEM;
    }

    rpc_callset_reply_buf = k_malloc(max_callset_size);
    if (!rpc_callset_reply_buf)
    {
        LOG_ERR("Error allocating memory for rpc_callset_reply_buf.");
        return -ENOMEM;
    }

    /** @brief Init the rpc object. */
    rpc.call_frame             = rpc_call_frame;
    rpc.reply_frame            = rpc_reply_frame;
    rpc.callsets               = callsets;
    rpc.callset_call_buf       = rpc_callset_call_buf;
    rpc.callset_call_buf_size  = max_callset_size;
    rpc.callset_reply_buf      = rpc_callset_reply_buf;
    rpc.callset_reply_buf_size = max_callset_size;
    rpc.num_callsets           = PROTORPC_ARRAY_LENGTH(callsets);

    return 0;
}

int main(void)
{
    int ret;

    LOG_INF("RPC demo app.");

    ret = NvParms_init();
    if (ret < 0)
    {
        LOG_ERR("NvParms module init error : %d", ret);
        return 0;
    }

    init_wifi();

    rpc_init();

    /* Start TCP Rcp server. */
    ret = TcpRpcServer_init(
        &tcp_rpc,
        &rpc,
        13001, 
        RPCSERVER_STACK_SIZE,
        20);
    if (ret < 0) LOG_ERR("Error initializing TcpRpcServer.");

    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
