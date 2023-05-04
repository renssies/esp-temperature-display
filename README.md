# ESP8266 Temperature Display
A rewrite of the neopixel based temperature display at [Pixelbar](https://pixelbar.nl) originally created by Ghostleyjim

The source code can be found in the `esp-temperature-display` folder.

# Requirements
- Arduino IDE 2.1 + ESP Boards File
- ESPTool (for original firmware)

# Reverting to original firmware
If you want to revert the display to the original firmware, use the following command in the folder of this repository. Replace `[PORT]` with the serial port the NodeMCU is connected to.

```
esptool.py -b 115200 --port [PORT] write_flash 0x00000 original_firmware.bin
```
