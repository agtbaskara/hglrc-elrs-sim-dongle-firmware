#include "crsf.h"

#include <string.h>

#define CRSF_ADDR_FLIGHT_CONTROLLER 0xC8
#define CRSF_MAX_FRAME_LEN 64
#define CRSF_FRAMETYPE_RC_CHANNELS 0x16

// Parser state: bytes of the frame being assembled
static uint8_t buf[CRSF_MAX_FRAME_LEN];
static uint8_t pos;

// CRSF CRC-8 (polynomial 0xD5), computed over frame type + payload.
static uint8_t crc8_d5(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : crc << 1;
    }
    return crc;
}

// Decode the RC_CHANNELS_PACKED payload: 16 channels of 11 bits each,
// packed little-endian into 22 bytes.
static void unpack_channels(crsf_state_t *state, const uint8_t *p) {
    uint32_t bits = 0;
    int nbits = 0;
    for (int ch = 0; ch < CRSF_NUM_CHANNELS; ch++) {
        while (nbits < 11) {
            bits |= (uint32_t)*p++ << nbits;
            nbits += 8;
        }
        state->channels[ch] = bits & 0x7FF;
        bits >>= 11;
        nbits -= 11;
    }
}

// Reset the parser and set all channels to centre until real data arrives.
void crsf_init(crsf_state_t *state) {
    memset(state, 0, sizeof(*state));
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++)
        state->channels[i] = CRSF_CHANNEL_MID;
    pos = 0;
}

// Byte-at-a-time frame parser. Only CRC-valid RC channels frames are used.
// Anything malformed is dropped and parsing resyncs on the next 0xC8.
void crsf_feed(crsf_state_t *state, uint8_t byte, uint32_t now_ms) {
    // Frame: [addr][len][type][payload...][crc], len counts type+payload+crc
    if (pos == 0 && byte != CRSF_ADDR_FLIGHT_CONTROLLER)
        return;
    if (pos == 1 && (byte < 2 || byte > CRSF_MAX_FRAME_LEN - 2)) {
        pos = 0;
        return;
    }

    buf[pos++] = byte;
    if (pos < 2 || pos < buf[1] + 2)
        return;

    uint8_t len = buf[1];
    pos = 0;
    if (crc8_d5(&buf[2], len - 1) != buf[len + 1])
        return;
    if (buf[2] == CRSF_FRAMETYPE_RC_CHANNELS && len - 2 >= 22) {
        unpack_channels(state, &buf[3]);
        state->last_channels_ms = now_ms;
    }
}
