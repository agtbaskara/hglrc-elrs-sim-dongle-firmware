// USB HID joystick device: 8 analog axes, no buttons.

#include "tusb.h"
#include "pico/unique_id.h"

#include "usb_descriptors.h"

// pid.codes test VID/PID, fine for personal use
#define USB_VID 0x1209
#define USB_PID 0x0001

// Device descriptor. bcdDevice 2.00 = HID-only version (1.00 had a serial
// port). The bump makes Windows re-read the device.
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0200,
    .iManufacturer = 0,  // none: Linux would show "<manufacturer> <product>"
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

// TinyUSB callback: return the device descriptor.
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// HID report descriptor: one joystick with 8 absolute 16-bit axes, range
// 0..2047, and no buttons. Must match joystick_report_t.
static const uint8_t desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x04,        // Usage (Joystick)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x09, 0x35,        //   Usage (Rz)
    0x09, 0x36,        //   Usage (Slider)
    0x09, 0x37,        //   Usage (Dial)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x07,  //   Logical Maximum (2047)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0xC0,              // End Collection
};

// TinyUSB callback: return the HID report descriptor.
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

enum { ITF_NUM_HID = 0, ITF_NUM_TOTAL };
#define EPNUM_HID 0x81
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

// Configuration descriptor: one HID joystick interface, polled every 1 ms.
static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                       EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1),
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
    "",                 // manufacturer (unused, iManufacturer = 0)
    "ELRS Sim Dongle",  // product
    serial_str,
    "ELRS Sim Dongle",  // HID interface
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

// Unused HID control requests (TinyUSB requires both callbacks).
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}
