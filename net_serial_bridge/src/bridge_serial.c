/*******************************************************************************
 *  @file: bridge_serial.c
 *
 *  @brief: Serial side of the bridge. A thread polls SerialPipe for received
 *  bytes, packs them into items and queues the items for the TCP thread.
*******************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "CheckCond.h"
#include "SerialPipe.h"
#include "bridge.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief Gate holding the receive thread until initialization completes. */
static K_SEM_DEFINE(init_sem, 0, 1);

/** @brief Pool of items. Only the receive thread allocates from it. */
K_MEM_SLAB_DEFINE_STATIC(item_pool, sizeof(struct bridge_item),
    CONFIG_APP_BRIDGE_POOL_DEPTH, 4);

/** @brief Items queued for the TCP thread, oldest first. */
static K_FIFO_DEFINE(s2t_fifo);

/** @brief Number of items dropped because the pool was full. */
static uint32_t drop_count;

/******************************************************************************
    item_alloc
*//**
    @brief Allocates an item. Drops the oldest queued item when the pool is
    full so a connecting client sees the most recent serial bytes.

    No app-level lock is needed. k_fifo and k_mem_slab are spinlock
    protected, this thread is the only allocator, and the TCP thread holds at
    most one item at a time.

    @return Returns the item, or NULL when every item is held by the TCP
    thread.
******************************************************************************/
static struct bridge_item *
item_alloc(void)
{
    struct bridge_item *item;
    struct bridge_item *old;

    if (k_mem_slab_alloc(&item_pool, (void **)&item, K_NO_WAIT) == 0)
    {
        return item;
    }

    old = k_fifo_get(&s2t_fifo, K_NO_WAIT);
    if (old)
    {
        k_mem_slab_free(&item_pool, (void *)old);

        drop_count++;
        if ((drop_count & 63) == 1)
        {
            LOG_WRN("Serial->TCP queue full, dropped %u items so far.",
                drop_count);
        }
    }

    if (k_mem_slab_alloc(&item_pool, (void **)&item, K_NO_WAIT) != 0)
    {
        return NULL;
    }
    return item;
}

/******************************************************************************
    rx_thread
*//**
    @brief Polls SerialPipe and queues received bytes.

    An item is queued when it is full, or when a whole poll period passes with
    no new bytes. A single empty read does not flush: over USB CDC-ACM bytes
    arrive in 64-byte packets, and flushing between packets would cut the
    pool capacity to one packet per item. Waiting bytes stay in SerialPipe's
    own fifo while the pool has no free item.
******************************************************************************/
static void
rx_thread(void *p1, void *p2, void *p3)
{
    struct bridge_item *cur = NULL;
    bool idle = false;

    (void)p1;
    (void)p2;
    (void)p3;

    /* Hold off until initialized, then re-post for any later waiter. */
    k_sem_take(&init_sem, K_FOREVER);
    k_sem_give(&init_sem);

    while (1)
    {
        int num;

        if (!cur)
        {
            cur = item_alloc();
            if (!cur)
            {
                k_msleep(CONFIG_APP_BRIDGE_SERIAL_POLL_MS);
                continue;
            }
            cur->len = 0;
        }

        num = SerialPipe_rxGet(cur->data + cur->len,
            sizeof(cur->data) - cur->len);
        if (num > 0)
        {
            cur->len += (uint16_t)num;
            idle = false;
            if (cur->len == sizeof(cur->data))
            {
                k_fifo_put(&s2t_fifo, cur);
                cur = NULL;
            }
            /* Keep draining while bytes flow. */
            continue;
        }

        if ((cur->len > 0) && idle)
        {
            /* Two empty reads one poll period apart: the sender paused. */
            k_fifo_put(&s2t_fifo, cur);
            cur = NULL;
        }

        idle = true;
        k_msleep(CONFIG_APP_BRIDGE_SERIAL_POLL_MS);
    }
}

K_THREAD_DEFINE(
    bridge_rx,
    CONFIG_APP_BRIDGE_SERIAL_STACK_SIZE,
    rx_thread,
    NULL,
    NULL,
    NULL,
    CONFIG_APP_BRIDGE_SERIAL_THREAD_PRIO,
    0,
    0);

/******************************************************************************
    bridge_serial_get
*//**
    @brief Takes the oldest queued item without blocking.
    @return Returns the item, or NULL when the queue is empty.
******************************************************************************/
struct bridge_item *
bridge_serial_get(void)
{
    return k_fifo_get(&s2t_fifo, K_NO_WAIT);
}

/******************************************************************************
    bridge_serial_free
*//**
    @brief Returns an item to the pool.
    @param[in] item  Item obtained from bridge_serial_get().
******************************************************************************/
void
bridge_serial_free(struct bridge_item *item)
{
    k_mem_slab_free(&item_pool, (void *)item);
}

/******************************************************************************
    bridge_serial_init
*//**
    @brief Initializes SerialPipe and starts the serial receive thread.
    @return Returns 0 on success, negative on error.
******************************************************************************/
int
bridge_serial_init(void)
{
    int ret;

    ret = SerialPipe_init();
    CHECK_COND_RETURN_MSG(ret < 0, ret, "SerialPipe init failed.");

    LOG_INF("Serial bridge: %u items x %u bytes, poll %u ms.",
        (uint32_t)CONFIG_APP_BRIDGE_POOL_DEPTH,
        (uint32_t)CONFIG_APP_BRIDGE_ITEM_SIZE,
        (uint32_t)CONFIG_APP_BRIDGE_SERIAL_POLL_MS);

    /* Release the receive thread. */
    k_sem_give(&init_sem);

    return 0;
}
