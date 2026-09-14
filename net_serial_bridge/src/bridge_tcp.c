/*******************************************************************************
 *  @file: bridge_tcp.c
 *
 *  @brief: TCP side of the bridge. The TcpServer thread forwards socket bytes
 *  to SerialPipe and drains the serial receive queue to the socket.
*******************************************************************************/
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "CheckCond.h"
#include "SerialPipe.h"
#include "TcpServer.h"
#include "TcpSocket.h"
#include "app.h"
#include "bridge.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief Size of the socket receive buffer handed to TcpServer. */
#define TCP_RX_BUF_SIZE     1024

/** @brief Sleep between SerialPipe_send() retries, ms. */
#define SEND_RETRY_MS       2

static TcpServer tcp_server;
static uint8_t tcp_rx_buf[TCP_RX_BUF_SIZE];

/******************************************************************************
    serial_send_wait
*//**
    @brief Queues bytes on SerialPipe, waiting a bounded time for pool space.

    Waiting here holds the TcpServer thread, which closes the TCP window
    while the UART drains. That is the intended backpressure.

    @param[in] data  Bytes to send.
    @param[in] len  Number of bytes. Must not exceed SerialPipe_sendSize().
    @return Returns 0 on success, negative on error or timeout.
******************************************************************************/
static int
serial_send_wait(const uint8_t *data, uint16_t len)
{
    uint32_t start = k_uptime_get_32();
    int ret;

    while ((ret = SerialPipe_send(data, len)) == -ENOMEM)
    {
        if ((k_uptime_get_32() - start) >= CONFIG_APP_BRIDGE_TX_WAIT_MS)
        {
            break;
        }
        k_msleep(SEND_RETRY_MS);
    }
    return ret;
}

/******************************************************************************
    bridge_callback
*//**
    @brief TcpServer callback. Runs on socket data and on every poll timeout.

    @param[in] server  TcpServer object (unused).
    @param[in] sock  Connected socket.
    @param[in] data  Received socket bytes.
    @param[in] len  Number of received bytes, 0 on poll timeout or peer close.
    @param[out] finished  Set to 1: close as soon as the peer has closed.
******************************************************************************/
static void
bridge_callback(void *server, int sock, uint8_t *data, uint16_t len,
    int *finished)
{
    size_t chunk_max = SerialPipe_sendSize();
    uint16_t off = 0;
    struct bridge_item *item;

    ARG_UNUSED(server);
    *finished = 1;

    /* TCP -> serial. */
    while (off < len)
    {
        uint16_t num = (uint16_t)MIN((size_t)(len - off), chunk_max);

        if (serial_send_wait(data + off, num) < 0)
        {
            LOG_WRN("Serial TX backlog, dropped %u bytes.",
                (unsigned int)(len - off));
            break;
        }
        off += num;
    }

    /* Serial -> TCP. */
    while ((item = bridge_serial_get()) != NULL)
    {
        int num = TcpSocket_write(sock, item->data, item->len);

        bridge_serial_free(item);
        if (num < 0)
        {
            /* Peer is gone. The next poll or read closes the socket. */
            break;
        }
    }
}

/******************************************************************************
    app_bridge_init
*//**
    @brief Starts the serial side, then the TCP server on the bridge port.
    @return Returns 0 on success, negative on error.
******************************************************************************/
int
app_bridge_init(void)
{
    int ret;

    ret = bridge_serial_init();
    CHECK_COND_RETURN_MSG(ret < 0, ret, "Bridge serial init failed.");

    ret = TcpServer_init(
        &tcp_server,
        CONFIG_APP_BRIDGE_TCP_PORT,
        tcp_rx_buf,
        sizeof(tcp_rx_buf),
        CONFIG_APP_BRIDGE_TCP_STACK_SIZE,
        "TCP Bridge",
        CONFIG_APP_BRIDGE_TCP_THREAD_PRIO,
        bridge_callback);
    CHECK_COND_RETURN_MSG(ret < 0, ret, "Bridge TcpServer init failed.");

    ret = TcpServer_setPollTimeout(&tcp_server, CONFIG_APP_BRIDGE_TCP_POLL_MS);
    CHECK_COND_RETURN_MSG(ret < 0, ret, "Bridge poll timeout rejected.");

    LOG_INF("Serial bridge listening on TCP port %u.",
        (unsigned int)CONFIG_APP_BRIDGE_TCP_PORT);

    return 0;
}
