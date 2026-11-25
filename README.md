# omnibot

Software, firmware, and hardware documentation for a 3-wheeled omnidirectional robot project (since 2020).

**Current Version:** `0.1.0` ([Changelog](CHANGELOG.md)) · **Web Flasher & Tools:** `https://kreier.github.io/omnibot/` *(enable GitHub Pages → Source: GitHub Actions in repository settings)*

## Overview

Omnibot separates **real-time drivetrain control** from **high-level perception and autonomy**:

- **Drivetrain Controller:** **LilyGO TTGO T-Display** (original ESP32, 240×135 ST7789 display, 4 MB flash) driving **3 omnidirectional motors via DRV8833** H-bridges and reading **3× motor Hall sensors**.
- **Expansion & Autonomy:**
  - **I2C port** for local drivetrain peripherals (e.g., IMU, current/voltage monitor).
  - **Serial (UART) port** to receive velocity/motion commands from an upper-level controller (such as a **Jetson Nano** with LiDAR/camera object detection) running on its **own separate power supply**.
- **Power Architecture:** **2S or 3S LiPo** traction battery with a **separate 5 V regulated rail** from the LiPo powering the T-Display (the board's onboard 1S Li-Ion JST connector/charger is unused).
- **Remote Control & Safety Cut-Off:** **PlayStation 3 DualShock 3 / Sixaxis** over Bluetooth Classic (works with both **original Sony** and **clone** controllers). Provides manual driving and an independent remote safety override (`SELECT` → `SAFE_STOP`, `START` → toggle `MAN`/`AUTO`) to cut off external Jetson Nano serial input at any time.

## Repository Structure

| Path | Description |
|---|---|
| [`firmware/omnibot_tdisplay/`](firmware/omnibot_tdisplay/README.md) | `v0.1.0` PlatformIO firmware for LilyGO T-Display + DualShock 3 (with automated Bluedroid SSP patch) |
| [`firmware/dualshock3/`](firmware/dualshock3/README.md) | DualShock 3 clone USB/Bluetooth investigation, `ds3read.py` / `ds3write.py`, and Pico W BTstack test |
| [`docs/`](docs/index.html) | GitHub Pages site with **ESP Web Tools** firmware flasher & experimental browser DS3 MAC tool |
| [`docs/architecture.md`](docs/architecture.md) | System, power, expansion, and control-authority architecture |
| [`docs/roadmap.md`](docs/roadmap.md) | Phased development outlines (`0.1.0` → `0.5.0`) |
| [`docs/ds3-pairing.md`](docs/ds3-pairing.md) | DualShock 3 pairing, clone `0x116` SSP fix, and WebUSB/WebHID compatibility details |
| [`docs/adr/README.md`](docs/adr/README.md) | Architecture Decision Records (ADRs) |
| [`.github/workflows/pages.yml`](.github/workflows/pages.yml) | GitHub Actions workflow that compiles the firmware and deploys GitHub Pages |

## Quick Start (`v0.1.0`)

### 1. Flash via Browser (No Toolchain Required)

1. Enable GitHub Pages (**Settings → Pages → Build and deployment → Source: GitHub Actions**) and push to `main`.
2. Open the published Pages site in **Chrome, Edge, or Opera**.
3. Click **Install Omnibot v0.1.0 to T-Display** and pick your USB serial port.
4. Read the **ESP32 BT MAC** displayed on the T-Display screen.

### 2. Build & Flash Locally with PlatformIO

```bash
cd firmware/omnibot_tdisplay
pio run -t upload
pio device monitor
```

### 3. Pair Your DualShock 3

Write the T-Display's Bluetooth MAC address into your DualShock 3 over USB using `ds3write.py` (see [`firmware/dualshock3/README.md`](firmware/dualshock3/README.md)) or the experimental browser tool on the Pages site, unplug USB, and press the **PS** button.

## License

[MIT](LICENSE)
