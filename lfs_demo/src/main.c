/** @brief This examples demonstrates:
    Using the devicetree for accessing a gpio led.
    Creating threads.
    Using event flags.
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define SLEEP_TIME_MS   100

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = "/lfs",
};

static struct fs_mount_t *mountpoint = &lfs_storage_mnt;

static int
setup_gpio(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        LOG_ERR("GPIO device is not ready.");        
        return -1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret != 0)
    {
        LOG_ERR("gpio_pin_configure returned %d", ret);
        return -1;
    }
    return 0;
}

static void
ls(const char *path)
{
    int ret;
    struct fs_dir_t dir;
    struct fs_dirent info;

    fs_dir_t_init(&dir);

    ret = fs_opendir(&dir, path);
    if (ret < 0)
    {
        LOG_ERR("Failed open dir %s: %d", path, ret);
        return;
    }

    LOG_INF("Directory listing: %s", path);
    while (1)
    {
        ret = fs_readdir(&dir, &info);
        if (ret == 0 && info.name[0])
        {
            LOG_INF("%c %u: %s",
                (info.type == FS_DIR_ENTRY_FILE) ? 'f' : 'd',
                (unsigned int)info.size,
                info.name);
        }
        else if (ret == 0 && !info.name[0])
        {
            fs_closedir(&dir);
            return;
        }
        else
        {
            LOG_ERR("fs_readdir returned %d.", ret);
            return;
        }
    }
}

static void
update_bootcount(void)
{
    struct fs_file_t fp;
    struct fs_dirent ent;
    int ret;
    unsigned int boot_count;
    ssize_t sz;
    const char path[] = "/lfs/boot_count.txt";

    fs_file_t_init(&fp);

    ret = fs_stat(path, &ent);
    if (ret == -ENOENT)
    {
        LOG_INF("Creating boot_count.txt 0x%08x", fp.mp);

        ret = fs_open(&fp, path, FS_O_RDWR | FS_O_CREATE);
        if (ret < 0)
        {
            LOG_ERR("Failed to create file: %d", ret);
            return;
        }

        boot_count = 0;

        ret = fs_write(&fp, &boot_count, sizeof(boot_count));
        if (ret < 0)
        {
            LOG_ERR("Failed on initial write: %d", ret);
            fs_close(&fp);
            return;
        }

        fs_close(&fp);
    }
    else if (ret < 0)
    {
        LOG_ERR("fs_stat returned %d.", ret);
        return;
    }

    ret = fs_open(&fp, path, FS_O_RDWR);
    if (ret < 0)
    {
        LOG_ERR("Failed on open: %d", ret);
        return;
    }

    ret = fs_read(&fp, &boot_count, sizeof(boot_count));
    if (ret < 0)
    {
        LOG_ERR("Failed on read: %d", ret);
        fs_close(&fp);
        return;
    }

    LOG_INF("boot_count = %u", boot_count);

    boot_count++;

    fs_seek(&fp, 0, FS_SEEK_SET);
    ret = fs_write(&fp, &boot_count, sizeof(boot_count));
    if (ret < 0)
    {
        LOG_ERR("Failed on write: %d", ret);
    }

    ret = fs_close(&fp);
    if (ret < 0)
    {
        LOG_ERR("Failed on write: %d", ret);
    }

}

static int
littlefs_flash_erase(unsigned int id)
{
    const struct flash_area *pfa;
    int rc;

    rc = flash_area_open(id, &pfa);
    if (rc < 0)
    {
        LOG_ERR("FAIL: unable to find flash area %u: %d\n", id, rc);
        return rc;
    }

    LOG_INF("Area %u at 0x%x on %s for %u bytes",
        id,
        (unsigned int)pfa->fa_off,
        pfa->fa_dev->name,
        (unsigned int)pfa->fa_size);

    /* Optional wipe flash contents */
    if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE))
    {
        rc = flash_area_flatten(pfa, 0, pfa->fa_size);
        LOG_ERR("Erasing flash area ... %d", rc);
    }

    flash_area_close(pfa);
    return rc;
}

static int
littlefs_mount(struct fs_mount_t *mp)
{
    int rc;

    rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
    if (rc < 0)
    {
        return rc;
    }

    LOG_INF("Mounting lfs.");
    rc = fs_mount(mp);
    if (rc < 0)
    {
        LOG_ERR("FAIL: mount id %" PRIuPTR " at %s: %d",
            (uintptr_t)mp->storage_dev,
            mp->mnt_point,
            rc);
        return rc;
    }

    return 0;
}

int main(void)
{
    int led_state = 1;
    int ret;
    struct fs_statvfs sbuf;
    int rc;

    LOG_INF("Littlefs demonstration app.");

    setup_gpio();

    rc = littlefs_mount(mountpoint);
    if (rc < 0)
    {
        return 0;
    }

    LOG_INF("%s mount success", mountpoint->mnt_point);

    rc = fs_statvfs(mountpoint->mnt_point, &sbuf);
    if (rc < 0)
    {
        LOG_ERR("FAIL: statvfs: %d", rc);
        rc = fs_unmount(mountpoint);
        LOG_ERR("Error on unmount: %s %d", mountpoint->mnt_point, rc);
        return 0;
    }

    LOG_INF("%s: bsize = %lu ; frsize = %lu ; blocks = %lu ; bfree = %lu",
            mountpoint->mnt_point,
            sbuf.f_bsize,
            sbuf.f_frsize,
            sbuf.f_blocks,
            sbuf.f_bfree);

    ls("/lfs");
    update_bootcount();

    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
