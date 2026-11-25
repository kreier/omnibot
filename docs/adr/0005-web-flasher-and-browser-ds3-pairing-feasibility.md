# ADR-0005: GitHub Pages Web Flasher and Browser-Based DS3 MAC Programming Feasibility

- **Status:** Accepted
- **Date:** 2026-11-25

## Context

Users and contributors should be able to flash precompiled Omnibot T-Display firmware directly from the project's GitHub Pages website without installing Arduino IDE or PlatformIO. Additionally, we evaluated whether the DualShock 3's target Bluetooth MAC address can also be programmed directly from the browser.

## Decision

1. **ESP32 Web Flasher (Fully Supported):** Use **ESP Web Tools** (backed by Web Serial on Chromium browsers over HTTPS on GitHub Pages) to flash the CI-compiled `bootloader.bin`, `partitions.bin`, `boot_app0.bin`, and `firmware.bin` directly to any T-Display over USB-Serial.
2. **Browser DS3 MAC Programmer (Experimental / Driver-Dependent):** Include a browser-based DualShock 3 MAC reader/writer using **WebUSB** (for `045e:028e` Xbox-mode clones) and **WebHID** (for `054c:0268` original Sony controllers), while clearly documenting OS driver constraints and retaining `ds3read.py` / `ds3write.py` as the primary fallback.

## Consequences

- **Positive:**
  - Zero-install ESP32 flashing from Chrome/Edge/Opera on Linux, macOS, and Windows.
  - Every push to `main` builds and publishes fresh firmware binaries automatically via GitHub Actions.
- **Negative / Constraints:**
  - **WebUSB cannot detach OS kernel drivers** (`xpad` on Linux, `xusb22.sys` on Windows). Clone DS3 programming in the browser fails with `Unable to claim interface` unless the user unbinds `xpad` on Linux or installs WinUSB via Zadig on Windows (macOS works out-of-the-box if no 360 driver is installed).
  - **WebUSB blocks USB HID class (`0x03`)**, and **WebHID** feature reports (`0xF2`/`0xF5`) on original DS3 controllers depend on whether the host OS HID driver permits undeclared feature report IDs.
  - Because of these browser security boundaries, PyUSB (`ds3write.py`) remains the most reliable cross-distro tool on Linux, and `SixAxisPairTool` / `DsHidMini` remains recommended for original controllers on Windows.
