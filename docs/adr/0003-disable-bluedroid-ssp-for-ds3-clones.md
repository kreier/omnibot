# ADR-0003: Disable Bluedroid SSP in `esp32-ps3` for DualShock 3 Clone Compatibility

- **Status:** Accepted
- **Date:** 2026-11-25

## Context

While an original Sony DualShock 3 paired and stayed connected to the T-Display, a DualShock 3 clone (`41:42:D6:DC:EF:EA`, USB `045e:028e`) connected over ACL (`status=0x0`) and then disconnected after ~2 seconds with GAP `ACL disconnect reason=0x116` (HCI `0x16`: *Connection Terminated by Local Host*). Testing the same clone on a Raspberry Pi Pico W with BTstack (`gap_set_security_level(LEVEL_0)`) succeeded, proving the clone radio worked and the ESP32 Bluedroid security negotiation was rejecting the link.

## Decision

Initialize Bluedroid in `Ps3Controller::begin()` using `esp_bluedroid_init_with_cfg(&cfg)` with `cfg.ssp_en = false` instead of `esp_bluedroid_init()` (which defaults to `ssp_en = true`). Apply this deterministically during PlatformIO builds via [`scripts/patch_ps3_ssp.py`](../../firmware/omnibot_tdisplay/scripts/patch_ps3_ssp.py) against `eyr1n/Fork of PS3 Controller Host@^1.1.1-idf5.4`.

## Consequences

- **Positive:**
  - Both original and clone DualShock 3 controllers connect and stream input reliably on Arduino-ESP32 v3.3.12 (ESP-IDF 5.5).
  - Using a PlatformIO `pre:` build script keeps the patch tracked in Git and ensures GitHub Actions builds produce the exact patched firmware without vendoring the entire library.
- **Negative / Constraints:**
  - Disables Bluetooth Secure Simple Pairing globally for Classic BT on this ESP32 firmware; acceptable because the ESP32 only pairs with DualShock 3 controllers via pre-shared MAC address.
  - Anyone compiling in Arduino IDE instead of PlatformIO must apply the 3-line edit manually to `Ps3Controller.cpp`.
