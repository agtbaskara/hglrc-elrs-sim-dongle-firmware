# hglrc-elrs-sim-dongle-firmware

Custom RP2040 firmware for the **HGLRC Flight Simulator Dongle (2.4G ELRS V3.0)**, a.k.a. HGLRC CRSF Joystick. It turns ELRS channels 1–8 into **8 analog axes** of a USB HID joystick for RC and FPV simulators.

| Channel | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Axis | X | Y | Z | Rx | Ry | Rz | Slider | Dial |

**Motivation:** with the stock HGLRC firmware, RealFlight Evolution somehow crashed, so I wrote this replacement firmware.

> [!WARNING]
> Use at your own risk. This is unofficial firmware, not affiliated with or supported by HGLRC. Flashing it replaces the stock firmware and may void your warranty. A backup of the stock firmware is available in [`backup/`](backup/).

Tested with:

- **Radio:** RadioMaster T8L (ELRS 2.4 GHz)
- **Simulators:** RealFlight Evolution, Liftoff, Liftoff: Micro Drones, Uncrashed: FPV Drone Simulator, The Drone Racing League Simulator
- **Build and flash:** Raspberry Pi 5 (Raspberry Pi OS / Debian 13 trixie, 64-bit)

## Hardware

Available e.g. from [Helimonster](https://helimonster.jp/?pid=180227901).

- RP2040, 8 MB flash. Receiver UART on **GPIO4 (TX) / GPIO5 (RX)**, CRSF at 420000 baud.
- Receiver: onboard **HGLRC Hermes 2.4GHz RX** (ESP8285 + SX1280).
- Possibly matching schematics (not verified against this board): [mikeneiderhauser/CRSFJoystick](https://github.com/mikeneiderhauser/CRSFJoystick/tree/master/hardware/Gerber/HGLRC_CRSFJoystick).

## Flash

1. Download `elrs_sim_dongle.uf2` from [Releases](../../releases) (or build it, see below).
2. Unplug the dongle, hold the **BOOTSEL** button, and plug it back in.
3. Drag `elrs_sim_dongle.uf2` onto the `RPI-RP2` drive that appears.

## Build from source

On Debian / Raspberry Pi OS:

```sh
sudo apt install git cmake python3 build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone --recursive https://github.com/agtbaskara/hglrc-elrs-sim-dongle-firmware.git
cd hglrc-elrs-sim-dongle-firmware
cmake -S firmware -B firmware/build
cmake --build firmware/build
```

This produces `firmware/build/elrs_sim_dongle.uf2`. Flash it as described in [Flash](#flash).

## Stock firmware backup

[`backup/`](backup/) has the original RP2040 and ESP8285 firmware, plus restore steps. To restore the joystick firmware, hold BOOTSEL and drag `backup/pico_full_8MB.uf2` onto the drive.

## Tools

- `tools/uart_bridge/`: USB to receiver UART bridge for reading or flashing the ESP8285 with `esptool`.
- `pip install -r tools/requirements.txt` installs `esptool` and `pyserial`.

## License

[MIT](LICENSE), except:

- `backup/` contains HGLRC's stock firmware, which is not covered by this license.
- `pico_sdk_import.cmake` is copied from the Raspberry Pi Pico SDK (BSD-3-Clause).

## References

- [mikeneiderhauser/CRSFJoystick](https://github.com/mikeneiderhauser/CRSFJoystick): the project HGLRC's stock firmware is based on.
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and [TinyUSB](https://github.com/hathach/tinyusb): used to build this firmware.
