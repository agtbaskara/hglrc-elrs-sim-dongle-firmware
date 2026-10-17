#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>

#define CRSF_NUM_CHANNELS 16

// Raw CRSF channel range (ELRS): 172..1811, centre 992
#define CRSF_CHANNEL_MIN 172
#define CRSF_CHANNEL_MID 992
#define CRSF_CHANNEL_MAX 1811

// Everything decoded from the receiver so far
typedef struct {
    uint16_t channels[CRSF_NUM_CHANNELS];  // raw 11-bit values
    uint32_t last_channels_ms;             // time of the last RC channels frame
} crsf_state_t;

// Reset parser state and centre all channels.
void crsf_init(crsf_state_t *state);

// Feed one byte from the receiver UART.
void crsf_feed(crsf_state_t *state, uint8_t byte, uint32_t now_ms);

#endif
