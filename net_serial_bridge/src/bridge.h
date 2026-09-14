/*******************************************************************************
 *  @file: bridge.h
 *
 *  @brief: Serial-to-TCP bridge internals shared by bridge_serial.c and
 *  bridge_tcp.c.
*******************************************************************************/
#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdint.h>
#include <zephyr/kernel.h>

/** @brief One chunk of received serial bytes queued for the TCP thread. */
struct bridge_item
{
    /** @brief Reserved for use by the kernel fifo. Must be the first member. */
    void *fifo_rsvd;
    /** @brief Number of valid bytes in data. */
    uint16_t len;
    /** @brief Received serial bytes. */
    uint8_t data[CONFIG_APP_BRIDGE_ITEM_SIZE];
};

/******************************************************************************
    bridge_serial_init
*//**
    @brief Initializes SerialPipe and starts the serial receive thread.
    @return Returns 0 on success, negative on error.
******************************************************************************/
int
bridge_serial_init(void);

/******************************************************************************
    bridge_serial_get
*//**
    @brief Takes the oldest queued item without blocking.

    Return the item with bridge_serial_free() after use.

    @return Returns the item, or NULL when the queue is empty.
******************************************************************************/
struct bridge_item *
bridge_serial_get(void);

/******************************************************************************
    bridge_serial_free
*//**
    @brief Returns an item to the pool.
    @param[in] item  Item obtained from bridge_serial_get().
******************************************************************************/
void
bridge_serial_free(struct bridge_item *item);

#endif /* BRIDGE_H */
