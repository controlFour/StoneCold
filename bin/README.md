# Flashing Stonecold Firmware

This guide explains how to flash the Stonecold firmware to your M5Stack Dial using the browser-based ESP Web Tool.

## Requirements

- Chrome, Edge, or another browser that supports Web Serial API
- USB-C cable connected to your M5Stack Dial
- The firmware files in this directory:
  - `bootloader.bin`
  - `partitions.bin`
  - `firmware.bin`

## Flashing Instructions

1. **Open ESP Web Tool**

   Go to: https://espressif.github.io/esptool-js/

2. **Connect your M5Stack Dial**

   - Plug in your M5Stack Dial via USB-C
   - Click **Connect** and select the serial port (usually shows as "USB Serial" or similar)

3. **Configure flash settings**

   - Set baud rate to **921600** (or 460800 if you experience issues)
   - Set flash mode to **DIO**
   - Set flash frequency to **80MHz**

4. **Add the firmware files with these addresses:**

   | File | Flash Address |
   |------|---------------|
   | `bootloader.bin` | `0x0` |
   | `partitions.bin` | `0x8000` |
   | `firmware.bin` | `0x10000` |

   Click **Add File** for each entry and browse to select the corresponding `.bin` file.

5. **Flash the firmware**

   Click **Program** to start flashing. The process takes about 30 seconds.

6. **Reset the device**

   After flashing completes, press the reset button on your M5Stack Dial or unplug and replug the USB cable.

## Troubleshooting

- **Can't connect**: Make sure no other application (like Arduino IDE or PlatformIO) is using the serial port
- **Flash fails**: Try a lower baud rate (460800 or 115200)
- **Device not recognized**: Install the CH340 or CP210x USB driver for your operating system
