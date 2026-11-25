# Omnibot System Architecture

> **Status:** Living architecture document (baseline `v0.1.0`). Detailed mechanical dimensions, wheel geometry, and final GPIO pin assignments will be added as the hardware build progresses.

## 1. High-Level Overview

Omnibot is a 3-wheeled omnidirectional robot built around a **split-control architecture**:

1. **Low-Level Real-Time Drivetrain Controller — LilyGO TTGO T-Display (ESP32)**
   - Original ESP32 (`ESP32-D0WD-Q6`, 4 MB flash) with integrated 240×135 ST7789 TFT display.
   - Reads quadrature/pulse **Hall-effect sensors** on all 3 drive motors.
   - Drives all 3 DC motors through **DRV8833** dual H-bridge drivers.
   - Hosts a **Bluetooth Classic** link to a **DualShock 3 / Sixaxis** controller (original or clone) for manual driving, live diagnostics on the TFT, and an **independent safety cut-off** (`SAFE_STOP` / manual override).
   - Exposes an **I2C port** for local low-level peripherals and a **Serial (UART) port** for velocity/pose commands from an upper-level autonomy computer.
2. **Optional High-Level Compute Board (e.g. NVIDIA Jetson Nano)**
   - Runs perception (LiDAR, camera, object detection, SLAM/navigation).
   - Powered from its **own independent power supply**.
   - Sends motion setpoints over **Serial (UART)** to the T-Display.
   - Cannot bypass the T-Display's DualShock 3 safety interlock: the operator can cut off external serial control remotely at any time.

```text
+-----------------------------------+          +------------------------------------+
| High-Level Compute (Optional)     |          | Operator Remote                    |
| e.g., Jetson Nano                 |          | PlayStation 3 DualShock 3 / Clone  |
| - LiDAR / Camera / Object Detect  |          | - Manual sticks & triggers         |
| - Own independent power supply    |          | - START: toggle MAN <-> AUTO       |
+-----------------+-----------------+          | - SELECT: immediate SAFE_STOP      |
                  |                            +-----------------+------------------+
                  | Serial (UART, common GND)                    | Bluetooth Classic
                  v                                              v (L2CAP HID, SSP=off)
+-----------------------------------------------------------------------------------+
| Drivetrain Controller: LilyGO T-Display (Original ESP32, 4MB Flash, 240x135 TFT) |
|                                                                                   |
|  [Control Authority Gate: SAFE_STOP | MANUAL_DS3 | AUTO_SERIAL_ARMED]             |
|       |                                                                           |
|       +--> 3-Wheel Omni Kinematics + Closed-Loop PID (Planned v0.2.0+)            |
|               ^                                 |                                 |
|               | Hall Sensors (3x motors)        v DRV8833 Motor Drivers (3x)      |
|  [I2C Port] --+--> Local Peripherals            +--> 3x Omni Drive Motors         |
+-----------------------------------------------------------------------------------+
                  ^                                              ^
                  | 5V Regulated Rail                            | Main Motor Power
        +---------+---------+                          +---------+---------+
        | 5V Buck Converter |<-------------------------| 2S or 3S LiPo     |
        +-------------------+                          +-------------------+
        (Note: T-Display onboard 1S Li-Ion JST connector/charger is unused)
```

## 2. Subsystems

### 2.1 Drivetrain Controller (T-Display ESP32)

| Component | Specification | Notes |
|---|---|---|
| MCU | ESP32 (original dual-core Xtensa LX6, 4 MB flash) | Bluetooth Classic + Wi-Fi capable |
| Display | 1.14" ST7789 IPS TFT, 240×135 | SPI (`MOSI=19, SCLK=18, CS=5, DC=16, RST=23, BL=4`) |
| Motor Drivers | DRV8833 dual H-bridge modules | Drives 3 motors via PWM (`IN1`/`IN2` per channel) |
| Wheel Feedback | 3× Hall-effect motor sensors | Hardware pulse/quadrature counting (ESP32 PCNT/ISR) |
| Remote Link | Bluetooth Classic HID (L2CAP) | `Fork of PS3 Controller Host` with `cfg.ssp_en = false` |

### 2.2 Power Architecture

- **Main Traction Pack:** **2S (7.4 V nominal) or 3S (11.1 V nominal) LiPo** battery powering the DRV8833 motor drivers and motors (note: verify DRV8833 supply limit of 10.8 V max when using a freshly charged 3S LiPo at 12.6 V, or regulate/clamp accordingly in the final motor power stage).
- **Logic Rail (T-Display):** A **separate 5 V regulated power rail** derived from the 2S/3S LiPo feeds the T-Display (keeping motor switching noise isolated from the ESP32 5 V input).
- **Onboard 1S Li-Ion Connector:** The TTGO T-Display includes a 1S Li-Ion JST battery connector and charger IC; this connector is **intentionally left unused** in Omnibot.
- **High-Level Compute Supply:** The Jetson Nano (or equivalent companion board) uses its **own separate power supply**, sharing only signal ground and UART lines (with appropriate 3.3 V logic-level compatibility).

### 2.3 Expansion Ports

- **I2C Port:** Reserved for local drivetrain/chassis peripherals (e.g., IMU for yaw stabilization, battery voltage/current monitor, range sensors).
- **Serial (UART) Port:** Receives velocity/control frames (`vx, vy, omega`) from an upstream controller (Jetson Nano, Raspberry Pi, etc.) and optionally reports odometry/status back.

### 2.4 Control Authority & Remote Safety Cut-Off

A core design requirement is that the **DualShock 3 remains an independent safety and override channel** over any autonomous controller:

| Mode | Trigger | Motor Output Source | External Serial Input |
|---|---|---|---|
| `SAFE_STOP` | `SELECT` button, Bluetooth disconnect, or fault | Zero / disabled | **Cut off (ignored)** |
| `MANUAL_DS3` | Default on connect, or `START` toggle | DualShock 3 sticks | **Cut off (ignored)** |
| `AUTO_SERIAL_ARMED` | `START` toggle while DS3 link is alive | Upstream Serial (e.g., Jetson Nano) | **Accepted** (watchdog-guarded) |

In `v0.1.0`, this state machine (`ControlMode`) is wired to the DualShock 3 `START` / `SELECT` buttons and Bluetooth disconnect fail-safe, and its active state is displayed in the T-Display header (`STOP`, `MAN`, `AUTO`) as the foundation for `v0.2.0+` motor driving and serial ingestion.

## 3. Implementation Status

| Version | Scope | Status |
|---|---|---|
| `0.1.0` | T-Display UI, DualShock 3 original & clone BT connection (`ssp_en = false`), `ControlMode` safety state foundation, CI + Web Flasher | **Implemented** |
| `0.2.0` | Pin map finalization, DRV8833 PWM driver layer, 3-wheel omni inverse kinematics | Planned |
| `0.3.0` | Hall-sensor velocity/odometry feedback + closed-loop PID per wheel | Planned |
| `0.4.0` | Jetson Nano UART command protocol + watchdog + I2C IMU/sensor integration | Planned |
