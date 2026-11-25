# DualShock 3 Setup, Clone Fix & Browser Pairing Guide

Omnibot uses a PlayStation 3 **DualShock 3 / Sixaxis** controller over **Bluetooth Classic** as its primary manual controller and remote safety cut-off.

## 1. How DualShock 3 Pairing Works

Unlike standard Bluetooth peripherals, a DualShock 3 **does not** pair via a normal Bluetooth inquiry/scan. Instead:

1. The controller stores **one target host Bluetooth MAC address** in internal memory.
2. When you press the **PS** button, the controller initiates a Bluetooth Classic connection *directly to that stored host MAC address*.
3. Therefore, pairing means **writing the ESP32's Bluetooth MAC address into the DualShock 3 over USB** (or configuring the ESP32 to spoof the MAC already stored in the controller via `Ps3.begin("XX:XX:XX:XX:XX:XX")`).

> **Tip:** Omnibot `v0.1.0` calls `Ps3.begin()` (using the ESP32's factory Bluetooth MAC) and prints that exact MAC address directly on the **T-Display screen** while waiting for a connection.

## 2. Original vs. Clone Controllers

| Behavior | Original Sony DS3 (`054c:0268`) | Tested Clone DS3 (`045e:028e`) |
|---|---|---|
| USB Identification | Sony PS3 HID gamepad | Microsoft Xbox 360 (`045e:028e`) |
| `SixAxisPairTool` (Windows) | Works | Often does **not** detect the clone |
| USB Report `0xF2` / `0xF5` | Supported | **Supported** even in Xbox 360 mode |
| ESP32 `esp32-ps3` default firmware | Connects | **Fails after ~2 s with `reason=0x116`** |
| ESP32 `esp32-ps3` with `cfg.ssp_en = false` | Connects | **Connects and works** |

### Why Clones Failed with `0x116` (and the Fix)

With stock `Fork of PS3 Controller Host` (`1.1.1-idf5.4`) on Arduino ESP32 core `3.3.12` (ESP-IDF `5.5.5`), a GAP callback logs:

```text
ACL connect 41:42:D6:DC:EF:EA status=0x0
ACL disconnect reason=0x116
```

`0x116` is HCI reason `0x16` (*Connection Terminated by Local Host*). The ESP32 Bluedroid stack had **Secure Simple Pairing (SSP)** enabled by default (`ssp_en = true`), and the clone controller never responded to the SSP security check.

Disabling SSP in `Ps3Controller::begin()` resolves the issue:

```cpp
esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
cfg.ssp_en = false;
if (esp_bluedroid_init_with_cfg(&cfg)) {
    log_e("esp_bluedroid_init failed");
    return false;
}
```

In this repository's PlatformIO project (`firmware/omnibot_tdisplay`), [`scripts/patch_ps3_ssp.py`](../firmware/omnibot_tdisplay/scripts/patch_ps3_ssp.py) applies this patch automatically before compilation.

## 3. Arduino IDE vs. PlatformIO Notes

- **PlatformIO (recommended):** Everything is pre-configured in [`firmware/omnibot_tdisplay/platformio.ini`](../firmware/omnibot_tdisplay/platformio.ini) — both the `TFT_eSPI` T-Display pinout (`USER_SETUP_ID=25`) and the `cfg.ssp_en = false` patch are handled automatically.
- **Arduino IDE:**
  1. In `TFT_eSPI/User_Setup_Select.h`, uncomment `#include <User_Setups/Setup25_TTGO_T_Display.h>` (and comment out the default `#include <User_Setup.h>`).
  2. In `Ps3Controller.cpp` (`Fork of PS3 Controller Host`), apply the `cfg.ssp_en = false` block shown above.
  3. Include `extern "C" bool btInUse() { return true; }` in the sketch.

## 4. Programming the DualShock 3 MAC Address

### Option A: Linux Python Scripts (Works for Original & Xbox-Mode Clones)

See [firmware/dualshock3/README.md](../firmware/dualshock3/README.md) for the complete `ds3read.py` and `ds3write.py` scripts using `python3-usb` (PyUSB). PyUSB can call `detach_kernel_driver(0)`, which allows it to send the `0xF5` control transfer even when the Linux `xpad` or `hid-sony` kernel driver has claimed the controller.

### Option B: Can You Program the DualShock 3 Over the Browser?

**Short answer: Partially — it depends on your controller type and OS USB driver, due to browser security rules.**

The GitHub Pages site includes an experimental **Browser DS3 MAC Programmer**, and here is what works and what is blocked by browsers:

1. **Xbox-Mode Clone (`045e:028e`) via WebUSB:**
   - Because `045e:028e` uses vendor-specific USB interface class `0xFF` (not HID class `0x03`), Chromium allows WebUSB to list it.
   - **However**, WebUSB **cannot detach an OS kernel driver** (`detach_kernel_driver` does not exist in WebUSB).
   - **Linux:** The kernel `xpad` driver binds automatically, so `claimInterface(0)` fails unless you first run `sudo modprobe -r xpad` (or unbind the device in sysfs) and grant udev permissions.
   - **Windows:** `xusb22.sys` binds automatically, so `claimInterface(0)` fails unless you temporarily bind the device to **WinUSB** using [Zadig](https://zadig.akeo.ie/) (which disables normal Xbox gamepad use until reverted).
   - **macOS:** Since macOS ships no built-in Xbox 360 kernel driver, WebUSB can claim `045e:028e` directly if no third-party driver (like 360Controller) is installed.
2. **Original Sony DS3 (`054c:0268`) via WebHID:**
   - WebUSB **always blocks** HID interfaces (`0x03` is a WebUSB protected interface class).
   - **WebHID** (`navigator.hid`) *can* open `054c:0268` and attempt `receiveFeatureReport(0xF2)` / `receiveFeatureReport(0xF5)` / `sendFeatureReport(0xF5, ...)`, **but** OS HID stacks (especially Windows) often reject or filter feature reports (`0xF2`/`0xF5`) that are not explicitly declared in the controller's USB HID Report Descriptor.

Because of these OS/browser sandboxes, the website provides the browser tool **alongside** the copy-ready `ds3write.py` / `SixAxisPairTool` fallback so users always have a guaranteed path.
