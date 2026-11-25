# Changelog

All notable changes to this project will be documented in this file.

## [0.1.0] - 2026-11-25

### Added
- Initial **LilyGO T-Display (ESP32)** PlatformIO firmware (`firmware/omnibot_tdisplay`) with:
  - 240×135 ST7789 TFT telemetry UI (waiting screen displaying ESP32 BT MAC, connected stick visualizers, L2/R2 analog bars, battery indicator, active buttons).
  - DualShock 3 / Sixaxis Bluetooth Classic support for both original Sony controllers and clones via automated `cfg.ssp_en = false` Bluedroid patch (`scripts/patch_ps3_ssp.py`).
  - `ControlMode` safety gate foundation (`SAFE_STOP`, `MANUAL_DS3`, `AUTO_SERIAL_ARMED`) controlled via `SELECT` (cut-off) and `START` (mode toggle), plus fail-safe on disconnect.
- System architecture documentation (`docs/architecture.md`), project roadmap (`docs/roadmap.md`), DualShock 3 pairing guide (`docs/ds3-pairing.md`), and ADRs (`docs/adr/`).
- GitHub Pages site (`docs/`) with ESP Web Tools firmware flasher and experimental WebUSB/WebHID DualShock 3 MAC programmer.
- GitHub Actions workflow (`.github/workflows/pages.yml`) to compile the firmware and deploy GitHub Pages.
