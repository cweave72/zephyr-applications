/*******************************************************************************
 *  @file: rpc.c
 *  
 *  @brief: Declarations and init for rpc.
*******************************************************************************/
#include "TcpRpcServer.h"
#include "ProtoRpc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

#include "TestRpc.h"
#include "TestRpc.pb.h"
#include "SystemRpc.h"
#include "SystemRpc.pb.h"
#include "RtosUtilsRpc.h"
#include "RtosUtilsRpc.pb.h"

static ProtoRpc_Callset_Entry callsets[] = {
    PROTORPC_ADD_CALLSET(0, SystemRpc_resolver, system_Callset),
    PROTORPC_ADD_CALLSET(1, RtosUtilsRpc_resolver, rtosutils_Callset),
};

static TcpRpcServer tcp_rpc;
static ProtoRpc rpc;

#define RPCSERVER_STACK_SIZE    4*1024
#define RPC_USE_STATIC  0

#if RPC_USE_STATIC == 0
static uint8_t *rpc_call_frame;
static uint8_t *rpc_reply_frame;
static uint8_t *rpc_callset_call_buf;
static uint8_t *rpc_callset_reply_buf;
#else
static uint8_t rpc_call_frame[1008];
static uint8_t rpc_reply_frame[1408];
static uint8_t rpc_callset_call_buf[1000];
static uint8_t rpc_callset_reply_buf[1408];
#endif

/******************************************************************************
    [docimport rpc_start_server]
*//**
    @brief Starts the RPC server thread.
******************************************************************************/
int
rpc_start_server(void)
{
    LOG_INF("Starting tcp rpc server on port %u.", CONFIG_TCPRPCSERVER_PORT);

    /* Start TCP Rcp server. */
    int ret = TcpRpcServer_init(
        &tcp_rpc,
        &rpc,
        CONFIG_TCPRPCSERVER_PORT,
        RPCSERVER_STACK_SIZE,
        20);
    if (ret < 0) LOG_ERR("Error initializing TcpRpcServer.");
    
    return ret;
}

/******************************************************************************
    [docimport rpc_init]
*//**
    @brief Performs rpc initializations.
******************************************************************************/
int
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

#if RPC_USE_STATIC == 0
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
#endif

    /** @brief Init the rpc object. */
    rpc.call_frame             = rpc_call_frame;
    rpc.reply_frame            = rpc_reply_frame;
    rpc.callsets               = callsets;
    rpc.callset_call_buf       = rpc_callset_call_buf;
    rpc.callset_call_buf_size  = sizeof(rpc_callset_call_buf);
    rpc.callset_reply_buf      = rpc_callset_reply_buf;
    rpc.callset_reply_buf_size = sizeof(rpc_callset_reply_buf);
    rpc.num_callsets           = PROTORPC_ARRAY_LENGTH(callsets);

    return 0;
}
