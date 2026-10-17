#include "tusb.h"
#include "pico/unique_id.h"

#define USB_VID 0x2E8A
#define USB_PID 0x000A  // Raspberry Pi Pico SDK CDC

// Device descriptor for a single CDC serial port (MISC/IAD class).
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

// TinyUSB callback: return the device descriptor.
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

// Configuration descriptor: one CDC interface pair.
static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

// TinyUSB callback: return the configuration descriptor.
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// USB strings. Index 3 (serial number) is filled from the flash unique ID.
static char serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
static const char *string_desc[] = {
    (const char[]){0x09, 0x04},  // English
    "Raspberry Pi",
    "ELRS Sim Dongle UART Bridge",
    serial_str,
    "ESP UART",
};

// TinyUSB callback: return string descriptor `index` as UTF-16.
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_str[32];
    uint8_t len;

    if (index == 0) {
        memcpy(&desc_str[1], string_desc[0], 2);
        len = 1;
    } else {
        if (index >= sizeof(string_desc) / sizeof(string_desc[0]))
            return NULL;
        if (index == 3 && !serial_str[0])
            pico_get_unique_board_id_string(serial_str, sizeof(serial_str));
        const char *str = string_desc[index];
        len = strlen(str);
        if (len > 31)
            len = 31;
        for (uint8_t i = 0; i < len; i++)
            desc_str[1 + i] = str[i];
    }
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
    return desc_str;
}
