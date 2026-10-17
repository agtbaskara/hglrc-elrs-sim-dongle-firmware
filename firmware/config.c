#include "config.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "crsf.h"

#define CONFIG_MAGIC 0x43445345  // "ESDC"
#define CONFIG_VERSION 1

// Stored in the last flash sector, far away from the firmware
#define CONFIG_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct {
    uint32_t magic;
    uint8_t version;
    config_t config;
    uint8_t checksum;
} stored_config_t;

config_t config;

static uint8_t checksum(const config_t *c) {
    const uint8_t *p = (const uint8_t *)c;
    uint8_t sum = 0;
    for (unsigned i = 0; i < sizeof(*c); i++)
        sum += p[i];
    return sum;
}

static bool valid(const config_t *c) {
    for (int i = 0; i < CONFIG_NUM_AXES; i++)
        if (c->axis_channel[i] > CRSF_NUM_CHANNELS)
            return false;
    for (int i = 0; i < CONFIG_NUM_BUTTONS; i++)
        if ((c->button_channel[i] & ~CONFIG_BUTTON_INVERT) > CRSF_NUM_CHANNELS)
            return false;
    return true;
}

void config_defaults(void) {
    memset(&config, 0, sizeof(config));
    for (int i = 0; i < CONFIG_NUM_AXES; i++)
        config.axis_channel[i] = i + 1;
}

void config_load(void) {
    const stored_config_t *s = (const stored_config_t *)(XIP_BASE + CONFIG_FLASH_OFFSET);
    if (s->magic == CONFIG_MAGIC && s->version == CONFIG_VERSION &&
        s->checksum == checksum(&s->config) && valid(&s->config))
        config = s->config;
    else
        config_defaults();
}

// Interrupts are off while flash is erased and written (about 50 ms), because
// the interrupt handlers run from flash. A few CRSF frames are lost meanwhile.
void config_save(void) {
    static uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));
    stored_config_t *s = (stored_config_t *)page;
    s->magic = CONFIG_MAGIC;
    s->version = CONFIG_VERSION;
    s->config = config;
    s->checksum = checksum(&config);

    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(CONFIG_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(irq);
}
