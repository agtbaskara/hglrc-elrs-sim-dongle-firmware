# hglrc-elrs-sim-dongle-firmware

Custom RP2040 firmware for the **HGLRC Flight Simulator Dongle (2.4G ELRS V3.0)**, a.k.a. HGLRC CRSF Joystick. It turns the dongle into a USB HID joystick with **8 analog axes and 16 buttons** for RC and FPV simulators. Which ELRS channel drives which axis or button can be changed over USB serial.

Default mapping (no buttons until you assign them):

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

On Linux, `tools/flash.sh` can also flash without pressing the button, as long as the dongle runs this firmware or the stock firmware. It needs `picotool` and `usbutils` installed, and your user in the `dialout` group.

```sh
tools/flash.sh <uf2> [port]    # port defaults to /dev/ttyACM0
```

## Configure

The dongle also shows up as a serial port (a COM port on Windows, `/dev/ttyACM0` on Linux). Open it with any serial terminal, such as PuTTY, MobaXterm, picocom or minicom. Any baud rate works except 1200, which reboots the dongle into BOOTSEL.

```sh
picocom /dev/ttyACM0
```

```
help                       list commands
show                       show which channel controls each axis and button
status                     show link state and all channel values
<axis>=<channel>           set which channel controls an axis
b<n>=<channel> [invert]    set which channel presses button n (1-16)
defaults                   axes on channels 1-8, no buttons
save                       keep the current mapping after unplugging
bootsel                    reboot into BOOTSEL for flashing
```

Axes are `x y z rx ry rz slider dial` (or `1`–`8`). A button is pressed when its channel is high (above 1625 µs), or low (below 1375 µs) with `invert`. Use `none` to switch an axis or button off.

```
> dial=none
> b1=ch9 b2=ch10
> show
Axes:    X=ch1 Y=ch2 Z=ch3 Rx=ch4 Ry=ch5 Rz=ch6 Slider=ch7 Dial=none
Buttons: B1=ch9 B2=ch10
> save
```

A new mapping takes effect right away. Type `save` to keep it after unplugging. The saved mapping also survives firmware updates. To back up a mapping, copy the two lines from `show`. Pasting them back restores it exactly.

## Troubleshooting

**A 3-position switch on channel 5 only gives low and high.** ExpressLRS reserves channel 5 (AUX1) for arming and always sends it as a 2-position switch, even if the radio's channel monitor shows the middle position. Put a 2-position switch on channel 5 and move the 3-position switch to another channel, or use an ELRS mode with full-resolution channels if your radio offers one. Check with `status`, which shows the value the dongle actually receives.

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

[`backup/`](backup/) has the original RP2040 and ESP8285 firmware, plus restore steps. To restore the joystick firmware, flash `backup/pico_full_8MB.uf2` like any other UF2 (BOOTSEL and drag, or `tools/flash.sh backup/pico_full_8MB.uf2`). It takes about 2.5 minutes because it rewrites the whole flash, which also erases a saved mapping. Copy your `show` lines first if you want to keep it.

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
