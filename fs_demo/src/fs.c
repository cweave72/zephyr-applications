/*******************************************************************************
 *  @file: fs.c
 *
 *  @brief: File system setup. Mounts the FsApi file system, logs the volume
 *  statistics and increments a boot counter file as a local smoke test.
*******************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "FsApi.h"

#include "app.h"

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

/** @brief Name of the boot counter file, relative to the mount point. */
#define BOOTCOUNT_NAME  "bootcount"

/******************************************************************************
    update_bootcount
*//**
    @brief Reads the boot counter file, increments the counter and writes it
    back. A missing file starts the counter at 0.
******************************************************************************/
static int
update_bootcount(void)
{
    char path[64];
    uint32_t count = 0;
    ssize_t ret;

    snprintk(path, sizeof(path), "%s/%s", FsApi_getMountPoint(),
        BOOTCOUNT_NAME);

    ret = FsApi_readFile(path, 0, &count, sizeof(count));
    if (ret == -ENOENT)
    {
        LOG_INF("%s does not exist. Starting at 0.", path);
        count = 0;
    }
    else if (ret != sizeof(count))
    {
        LOG_WRN("Read of %s returned %d. Starting at 0.", path, (int)ret);
        count = 0;
    }

    count++;

    ret = FsApi_writeFile(path, 0, &count, sizeof(count), FS_O_CREATE);
    if (ret != sizeof(count))
    {
        LOG_ERR("Write of %s returned %d.", path, (int)ret);
        return (ret < 0) ? (int)ret : -EIO;
    }

    LOG_INF("%s = %u", path, count);
    return 0;
}

/******************************************************************************
    app_fs_init
*//**
    @brief Mounts the file system and runs the boot counter.
******************************************************************************/
int
app_fs_init(void)
{
    struct fs_statvfs stat;
    int ret;

    ret = FsApi_init();
    if (ret < 0)
    {
        LOG_ERR("FsApi_init failed: %d", ret);
        return ret;
    }

    ret = FsApi_getInfo(&stat);
    if (ret < 0)
    {
        LOG_ERR("FsApi_getInfo failed: %d", ret);
        return ret;
    }

    LOG_INF("%s: bsize = %lu; frsize = %lu; blocks = %lu; bfree = %lu",
        FsApi_getMountPoint(), stat.f_bsize, stat.f_frsize, stat.f_blocks,
        stat.f_bfree);

    return update_bootcount();
}
