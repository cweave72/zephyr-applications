# Demo of flash access

Currently, this does not work.
Hangs when cache is disabled deep in the call stack.  Specifically, hangs in:
`deps/modules/hal/espressif/zephyr/port/host_flash/cache_utils.c:68`

call stack:

```
main.c: spi_flash_mmap()                  (hal/espressif/components/spi_flash/flash_mmap.c)
-> esp_mmu_map()                  (hal/espressif/components/esp_mm/esp_mmu_map.c)
-> s_do_mapping()                 (same as above)
-> spi_flash_disable_interrupts_caches_and_other_cpu() (hal/espressif/components/spi_flash/cache_utils.c)
-> spi_flash_disable_cache()      (hal/espressif/zephyr/port/host_flash/cache_utils.c)
-> DPORT_SET_PERI_REG_BITS(): line 68
```
