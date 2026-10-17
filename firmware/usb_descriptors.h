#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

#define JOYSTICK_NUM_AXES 8
#define JOYSTICK_NUM_BUTTONS 16
#define JOYSTICK_AXIS_MAX 2047

// Must match the HID report descriptor in usb_descriptors.c
typedef struct __attribute__((packed)) {
    uint16_t axes[JOYSTICK_NUM_AXES];  // X, Y, Z, Rx, Ry, Rz, Slider, Dial
    uint16_t buttons;                  // bit 0 = button 1
} joystick_report_t;

#endif
