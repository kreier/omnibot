# ADR-0002: DualShock 3 as Independent Remote Override & Safety Cut-Off

- **Status:** Accepted
- **Date:** 2026-11-25

## Context

When testing autonomous driving from an upstream controller (e.g., a Jetson Nano running LiDAR/camera object detection over UART), software bugs, sensor glitches, or runaway velocity commands can cause physical collisions. Relying solely on SSH/Wi-Fi to the companion computer is too slow and fragile as an emergency stop.

## Decision

Make the **DualShock 3 Bluetooth link** to the T-Display the authoritative safety and mode-gating layer:

1. Introduce an explicit `ControlMode` state machine (`SAFE_STOP`, `MANUAL_DS3`, `AUTO_SERIAL_ARMED`).
2. `SELECT` immediately forces `SAFE_STOP` and cuts off external serial commands.
3. `START` toggles between `MANUAL_DS3` and `AUTO_SERIAL_ARMED`.
4. Disconnecting the DualShock 3 automatically drops to `SAFE_STOP`.
5. Future `v0.2.0+` motor drivers will refuse any external serial command unless `controlMode == AUTO_SERIAL_ARMED` and the serial watchdog is fresh.

## Consequences

- **Positive:**
  - Operator retains instant, sub-second wireless override and emergency cut-off independent of the Jetson Nano OS or network.
  - `v0.1.0` validates the controller link and mode-switching UI before motors are even wired.
- **Negative / Constraints:**
  - Autonomous driving requires an active DualShock 3 heartbeat (or an explicit physical/config bypass switch if headless operation without a gamepad is ever desired later).
