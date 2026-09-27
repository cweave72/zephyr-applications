/*******************************************************************************
 *  @file: fs.c
 *
 *  @brief: File system setup. FsApi mounts /flash (a flash partition) from
 *  the fstab node, and /ram (littlefs on a RAM disk) from the mount below.
 *  The file logs the volume statistics of each mount and increments a boot
 *  counter file on /flash as a local smoke test.
*******************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/littlefs.h>

#include "FsApi.h"

#include "app.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief The boot counter file. */
#define BOOTCOUNT_PATH  "/flash/bootcount"

/* littlefs on the RAM disk "RAM" (ramdisk0 in boards/<board>.overlay). On a
   disk, littlefs uses one sector (512 B) as the block, read, prog and cache
   size, and needs a lookahead of 4 blocks. */
FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(ram_lfs, 4, 512, 512, 512, 2048);

static struct fs_mount_t ram_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &ram_lfs,
    .storage_dev = (void *)"RAM",
    .mnt_point = "/ram",
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

/******************************************************************************
    update_bootcount
*//**
    @brief Reads the boot counter file, increments the counter and writes it
    back. A missing file starts the counter at 0.
******************************************************************************/
static int
update_bootcount(void)
{
    uint32_t count = 0;
    ssize_t ret;

    ret = FsApi_readFile(BOOTCOUNT_PATH, 0, &count, sizeof(count));
    if (ret == -ENOENT)
    {
        LOG_INF("%s does not exist. Starting at 0.", BOOTCOUNT_PATH);
        count = 0;
    }
    else if (ret != sizeof(count))
    {
        LOG_WRN("Read of %s returned %d. Starting at 0.", BOOTCOUNT_PATH,
            (int)ret);
        count = 0;
    }

    count++;

    ret = FsApi_writeFile(BOOTCOUNT_PATH, 0, &count, sizeof(count),
        FS_O_CREATE);
    if (ret != sizeof(count))
    {
        LOG_ERR("Write of %s returned %d.", BOOTCOUNT_PATH, (int)ret);
        return (ret < 0) ? (int)ret : -EIO;
    }

    LOG_INF("%s = %u", BOOTCOUNT_PATH, count);
    return 0;
}

/******************************************************************************
    app_fs_init
*//**
    @brief Mounts /flash and /ram, logs their statistics and runs the boot
    counter.
******************************************************************************/
int
app_fs_init(void)
{
    struct fs_statvfs stat;
    int ret;
    int k;

    ret = FsApi_init();
    if (ret < 0)
    {
        LOG_ERR("FsApi_init failed: %d", ret);
    }

    ret = FsApi_addMount(&ram_mnt);
    if (ret < 0)
    {
        LOG_ERR("Mount of %s failed: %d", ram_mnt.mnt_point, ret);
    }

    for (k = 0; k < FsApi_getMountCount(); k++)
    {
        const char *mnt = FsApi_getMountPoint(k);

        ret = FsApi_getInfo(mnt, &stat);
        if (ret < 0)
        {
            LOG_ERR("FsApi_getInfo(%s) failed: %d", mnt, ret);
            continue;
        }
        LOG_INF("%s: bsize = %lu; frsize = %lu; blocks = %lu; bfree = %lu",
            mnt, stat.f_bsize, stat.f_frsize, stat.f_blocks, stat.f_bfree);
    }

    return update_bootcount();
}
