#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define CONFIG_NUM_AXES 8
#define CONFIG_NUM_BUTTONS 16
#define CONFIG_BUTTON_INVERT 0x80  // flag in button_channel: pressed when low

// Channel mapping. Channels are numbered 1..16, 0 means unused.
typedef struct {
    uint8_t axis_channel[CONFIG_NUM_AXES];      // unused axes stay centred
    uint8_t button_channel[CONFIG_NUM_BUTTONS];  // channel, optionally | CONFIG_BUTTON_INVERT
} config_t;

extern config_t config;

// Defaults: axes on channels 1-8, no buttons.
void config_defaults(void);

// Load the mapping from flash, or the defaults if none is stored.
void config_load(void);

// Store the current mapping in flash.
void config_save(void);

#endif
