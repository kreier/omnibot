# ADR-0001: Use LilyGO T-Display (ESP32) as Dedicated Drivetrain Controller

- **Status:** Accepted
- **Date:** 2026-11-25

## Context

Omnibot requires deterministic low-level control of three omnidirectional DC motors via DRV8833 H-bridge drivers, simultaneous counting of three motor Hall-effect sensors, an I2C bus for local chassis sensors, a UART link to an optional high-level compute board (such as a Jetson Nano), and a built-in diagnostic screen for field debugging without a laptop.

## Decision

Use the **LilyGO TTGO T-Display** (original ESP32-D0WD-Q6, 4 MB flash, integrated 240×135 ST7789 TFT display) as the dedicated real-time drivetrain MCU.

## Consequences

- **Positive:**
  - Original ESP32 includes **Bluetooth Classic (BR/EDR)**, which is required by DualShock 3 controllers (newer BLE-only chips such as ESP32-S3 or ESP32-C3 cannot host a DualShock 3 directly).
  - Integrated 240×135 display provides immediate visual status (BT MAC address, controller state, safety mode, future motor RPM/battery telemetry) right on the robot.
  - Hardware PCNT/timers handle 3 Hall sensor channels cleanly alongside PWM motor outputs, I2C, and UART.
- **Negative / Constraints:**
  - The SPI TFT (`GPIO 4, 5, 16, 18, 19, 23`) and onboard buttons (`GPIO 0, 35`) reduce the remaining free GPIO count, requiring careful pin budgeting for 6 motor PWM lines, 3 Hall inputs, I2C, and UART.
  - 4 MB flash requires keeping Bluetooth + TFT + control code within standard partition limits.
