#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "crsf.h"

// Serial configuration console on the USB serial port. Call every main loop.
// Returns true when the user asked to reboot into BOOTSEL.
bool cli_task(const crsf_state_t *crsf, bool link_up);

#endif
