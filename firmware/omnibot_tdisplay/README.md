# Omnibot T-Display Firmware (`v0.1.0`)

PlatformIO project for the **LilyGO TTGO T-Display** (original ESP32, 4 MB flash, 240×135 ST7789 display).

## Features in `0.1.0`

- Connects to **original and clone DualShock 3 / Sixaxis** controllers over Bluetooth Classic.
- Automatically applies the Bluedroid `cfg.ssp_en = false` patch in [`scripts/patch_ps3_ssp.py`](scripts/patch_ps3_ssp.py) before compilation so clone controllers do not disconnect after 2 s with `reason=0x116`.
- Configures `TFT_eSPI` for TTGO T-Display (`USER_SETUP_ID=25`) directly via `platformio.ini` build flags (no manual library file edits needed).
- Displays the ESP32's Bluetooth MAC address on the waiting screen so you can pair any DualShock 3 directly to a freshly flashed board.
- Implements the `ControlMode` safety foundation (`SAFE_STOP`, `MANUAL_DS3`, `AUTO_SERIAL_ARMED`) toggled with `START` and cut off immediately with `SELECT` or link loss.

## Build & Flash

```bash
cd firmware/omnibot_tdisplay
pio run -t upload
pio device monitor
```

Or flash precompiled binaries directly from the project's GitHub Pages website using **ESP Web Tools**.
