# Stock firmware backup — HGLRC CRSF Joystick

Taken 2026-10-17. Each dump was read twice and the SHA256 matched (see `SHA256SUMS`).

## RP2040 (joystick MCU)

- 8 MB flash (W25Q64), only the first 80 KB used.
- Stock firmware: HGLRC fork of CRSFJoystick, PlatformIO + Arduino Mbed core, USB VID:PID `2e8a:00c0`, HID joystick + CDC with a fake Betaflight CLI. Its `serialpassthrough` does not reach the ESP8285 (it loops TX back to USB).

| File | Contents |
|---|---|
| `pico_full_8MB.bin` | Raw full flash, `0x10000000`–`0x10800000` |
| `pico_full_8MB.uf2` | Same as UF2 |
| `pico_app_80KB.bin` | First 80 KB (used region only) |

Restore: hold BOOTSEL, plug in, then either drag `pico_full_8MB.uf2` onto the `RPI-RP2` drive or run `picotool load -v -x pico_full_8MB.uf2`.

## ESP8285 (onboard ELRS receiver)

- ESP8285H16, 2 MB embedded flash, 26 MHz crystal, MAC `04:83:08:f5:c4:1b`.
- ExpressLRS **3.2.0 (8d3388)**, target `HGLRC_HERMES_2400_RX` ("HGLRC Hermes 2.4GHz RX").

| File | Contents |
|---|---|
| `esp8285_full_2MB.bin` | Raw full flash, `0x000000`–`0x200000` (includes ELRS config/bind data) |

Read/restore path: flash `tools/uart_bridge` (build it like `firmware/`) to the Pico (USB serial to UART1 on GPIO4/5), hold the receiver's BOOT button while it powers up, then:

```
esptool -p /dev/ttyACM0 -b 460800 --before no-reset --after no-reset write-flash 0 esp8285_full_2MB.bin
```
