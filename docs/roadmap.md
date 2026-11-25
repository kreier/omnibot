# Project Outlines & Roadmap

This document outlines the phased development plan for Omnibot, moving from a verified wireless safety/control baseline (`0.1.0`) to a full closed-loop omnidirectional drivetrain with autonomous companion-computer integration.

## Milestone 0.1.0 — T-Display + DualShock 3 Foundation (Current)

**Goal:** Establish a reproducible firmware build, automated web-flasher deployment, and a rock-solid Bluetooth Classic link to both original and clone DualShock 3 controllers.

- [x] PlatformIO project targeting `lilygo-t-display` (ESP32, 4 MB flash, 240×135 ST7789).
- [x] Zero-manual-edit `TFT_eSPI` build configuration via `platformio.ini` flags (`USER_SETUP_ID=25`).
- [x] Automated Bluedroid SSP disable patch (`cfg.ssp_en = false`) so DualShock 3 clones connect reliably without dropping at 2 s with `reason=0x116`.
- [x] Live 25 FPS T-Display dashboard showing ESP32 BT MAC (for pairing), DualShock 3 stick positions, L2/R2 analog bars, battery state, active buttons, and `ControlMode` (`STOP` / `MAN` / `AUTO`).
- [x] Remote safety cut-off foundation (`SELECT` → `SAFE_STOP`, `START` → `MAN`/`AUTO` toggle, link-loss → `SAFE_STOP`).
- [x] GitHub Actions workflow building `0.1.0` binaries and publishing a GitHub Pages Web Flasher + browser pairing tool.

## Milestone 0.2.0 — Drivetrain Power Stage & Open-Loop Kinematics

**Goal:** Drive all three omnidirectional wheels smoothly from the DualShock 3 sticks.

- [ ] Finalize ESP32 GPIO assignments for 3× DRV8833 channels (avoiding T-Display SPI/backlight/button pins).
- [ ] Implement 3-wheel omnidirectional inverse kinematics ($v_x, v_y, \omega \rightarrow \omega_1, \omega_2, \omega_3$).
- [ ] Add deadzone, exponential stick curves, and slew-rate limiting.
- [ ] Enforce `SAFE_STOP` at the hardware PWM layer (coast/brake output disable).

## Milestone 0.3.0 — Hall Sensors & Closed-Loop Velocity Control

**Goal:** Read all three motor Hall sensors and regulate wheel speeds accurately.

- [ ] Configure ESP32 PCNT / interrupt counters for all 3 motor Hall channels.
- [ ] Measure ticks-per-revolution and wheel diameter calibration constants.
- [ ] Add per-wheel PID velocity controllers and live RPM/error telemetry page on the T-Display.

## Milestone 0.4.0 — Jetson Nano Serial Protocol & I2C Peripherals

**Goal:** Enable autonomous navigation from an externally powered compute board while preserving remote human override.

- [ ] Define a framed, checksummed UART command/telemetry protocol (e.g., COBS/JSON or binary packet with sequence + CRC16).
- [ ] Add command watchdog timeout (e.g., 200 ms) that automatically drops `AUTO_SERIAL_ARMED` to `SAFE_STOP` if the Jetson stops transmitting.
- [ ] Gate serial drive commands strictly behind `ControlMode::AUTO_SERIAL_ARMED` (any DS3 stick deflection or `SELECT` immediately overrides/cuts off external control).
- [ ] Add I2C bus initialization and first peripheral driver (e.g., BNO08x/MPU6050 IMU or INA219 battery monitor).

## Milestone 0.5.0 — Mechanical & Electrical CAD / Wiring Documentation

**Goal:** Complete full hardware reproducibility.

- [ ] Publish wiring diagram (2S/3S LiPo, 5 V rail, DRV8833, Hall sensors, UART isolation/level-shifting, I2C header).
- [ ] Document chassis geometry, wheel angles (120°), and 3D-printed/cut parts.
