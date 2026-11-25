# ADR-0004: 2S/3S LiPo Traction Power with Separate 5V Logic Rail and Isolated Compute Supply

- **Status:** Accepted
- **Date:** 2026-11-25

## Context

Three DC omni motors driven through DRV8833 H-bridges generate significant current spikes and inductive noise, while the Jetson Nano (plus LiDAR/camera) draws high peak currents and operates on its own power budget. Meanwhile, the TTGO T-Display includes an onboard 1S Li-Ion JST connector and charger circuit.

## Decision

1. Power the drivetrain from a **2S or 3S LiPo** battery pack.
2. Supply the T-Display from a **dedicated 5 V step-down rail** derived from the 2S/3S LiPo (leaving the onboard 1S Li-Ion battery connector/charger **unused**).
3. Power the upstream compute board (Jetson Nano + LiDAR/camera) from its **own separate power supply**, connecting only UART TX/RX and a common signal reference ground (or isolated UART) to the T-Display.

## Consequences

- **Positive:**
  - Prevents Jetson Nano brownouts or high-current surges from affecting the drivetrain MCU, and prevents motor stalls from crashing the companion computer.
  - Avoids managing a secondary 1S Li-Ion cell on the T-Display.
- **Negative / Constraints:**
  - Note for hardware wiring: DRV8833 absolute max `VM` is typically **10.8 V** (`11.8 V` abs max), whereas a fully charged 3S LiPo reaches **12.6 V**. Using a 2S LiPo (up to 8.4 V) stays directly within DRV8833 ratings, while a 3S LiPo requires a suitable buck pre-regulator for the DRV8833 `VM` rail (to be specified in the upcoming hardware schematic).
