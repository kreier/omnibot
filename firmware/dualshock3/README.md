# Clone DualShock 3 over Bluetooth

Findings from 2026-11-25: using a clone DualShock 3 (Sixaxis) that identifies as an Xbox 360 controller over USB with an ESP32 and a Raspberry Pi Pico W.

## Summary

| Test | Result |
|---|---|
| Windows 11, USB | Works as an Xbox 360 controller |
| SixAxisPairTool | Does not find the controller |
| Linux, USB | `045e:028e` (Xbox 360), no PS3 mode observed |
| Read PS3 pairing reports in Xbox mode | **Works** |
| Write host address in Xbox mode | **Works** |
| ESP32 with `Ps3Controller.h`, Bluetooth | **Fails**: connects, then the ESP32 side drops the link |
| Pico W with BTstack, Bluetooth | **Works**, including buttons and sticks |

## Addresses

| Device | Bluetooth address |
|---|---|
| Clone controller | `41:42:D6:DC:EF:EA` |
| Host originally stored in the clone | `41:43:00:00:03:A6` |
| ESP32 (TTGO T-Display) | `10:52:1C:66:50:7E` |
| Pico W | `28:CD:C1:09:3C:74` |

## Background

A DualShock 3 is not paired through a normal Bluetooth scan. The controller stores one **host address** and connects to it when PS is pressed. That address is written over USB.

## USB behaviour

```text
idVendor=045e, idProduct=028e, bcdDevice= 1.10
Product: Controller
Manufacturer: Sony
```

Holding PS + Start or PS + Select while plugging in did not change the USB ID. A `bluetoothctl` `scan bredr` showed no device identified as the controller.

## Reading and writing the addresses (Linux)

Although it runs in Xbox mode, the clone answers the PS3 feature reports:

| Report | Bytes (counted from 0) | Content |
|---|---|---|
| `0xF2` | 4–9 | Controller address |
| `0xF5` | 2–7 | Stored host address |

Install PyUSB:

```bash
sudo apt install python3-usb
```

Both scripts detach the kernel gamepad driver. Replug the controller to restore it.

### ds3read.py

```python
import usb.core
d = usb.core.find(idVendor=0x045e, idProduct=0x028e)
if d is None:
    raise SystemExit("Controller not found")
if d.is_kernel_driver_active(0):
    d.detach_kernel_driver(0)
for rid in (0xF2, 0xF5):
    try:
        r = d.ctrl_transfer(0xA1, 0x01, 0x0300 | rid, 0, 64, 1000)
        print(hex(rid), bytes(r).hex(':'))
    except usb.core.USBError as e:
        print(hex(rid), e)
```

### ds3write.py

```python
import sys, usb.core
if len(sys.argv) != 2:
    raise SystemExit("Usage: ds3write.py XX:XX:XX:XX:XX:XX")
mac = bytes.fromhex(sys.argv[1].replace(":", ""))
if len(mac) != 6:
    raise SystemExit("Address must be 6 bytes")
d = usb.core.find(idVendor=0x045e, idProduct=0x028e)
if d is None:
    raise SystemExit("Controller not found")
if d.is_kernel_driver_active(0):
    d.detach_kernel_driver(0)

def host():
    r = d.ctrl_transfer(0xA1, 0x01, 0x03F5, 0, 64, 1000)
    return bytes(r[2:8]).hex(':')

print("before:", host())
d.ctrl_transfer(0x21, 0x09, 0x03F5, 0, b'\x01\x00' + mac, 1000)
print("after: ", host())
```

Usage:

```bash
sudo python3 ds3read.py
sudo python3 ds3write.py 10:52:1C:66:50:7E   # target host address
```

## ESP32: connection fails

Setup:

- TTGO / LilyGO T-Display (ESP32-D0WD-Q6, revision 1.0)
- Arduino ESP32 core 3.3.12 (ESP-IDF 5.5.5)
- Library: Fork of PS3 Controller Host 1.1.1-idf5.4
- An original DualShock 3 has connected to this ESP32

The four controller LEDs blink fast and turn off after about 2 seconds. Log from a GAP callback:

```text
ACL connect 41:42:D6:DC:EF:EA status=0x0
ACL disconnect reason=0x116
```

`0x116` is HCI error `0x16`, "connection terminated by local host": the ESP32's Bluetooth stack ended the link.

### Logging callback

The include must be at the top of the sketch. Otherwise the Arduino-generated function declaration for `gapLog` fails to compile.

```cpp
#include <esp_gap_bt_api.h>

void gapLog(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT) {
    uint8_t *a = param->acl_conn_cmpl_stat.bda;
    Serial.printf("ACL connect %02X:%02X:%02X:%02X:%02X:%02X status=0x%X\n",
                  a[0], a[1], a[2], a[3], a[4], a[5],
                  param->acl_conn_cmpl_stat.stat);
  } else if (event == ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT) {
    Serial.printf("ACL disconnect reason=0x%X\n",
                  param->acl_disconn_cmpl_stat.reason);
  } else {
    Serial.printf("GAP event %d\n", (int)event);
  }
}

// in setup(), directly after Ps3.begin():
//   esp_bt_gap_register_callback(gapLog);
```

## Pico W: connection works

Sketch: [`pico_ds3_test/pico_ds3_test.ino`](pico_ds3_test/pico_ds3_test.ino)

1. Add the board URL `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. Install **Raspberry Pi Pico/RP2040** by Earle F. Philhower and select **Raspberry Pi Pico W** (sketch compiled with version 6.4.0).
3. Set **Tools → IP/Bluetooth Stack → IPv4 + Bluetooth**.
4. Upload and note the Pico address from the serial monitor (send `a` to repeat it).
5. Write that address to the controller with `ds3write.py`, unplug it and press PS.

Result:

```text
Connection request from 41:42:D6:DC:EF:EA
ACL connect 41:42:D6:DC:EF:EA status 0x00
L2CAP request, PSM 0x11
L2CAP PSM 0x11 opened, status 0x00
L2CAP request, PSM 0x13
L2CAP PSM 0x13 opened, status 0x00
Enable command sent, result 0x00
LED command sent, result 0x00
#44 buttons 00 00 00  L 128,128  R 128,128
#641 buttons 00 00 00  L   9, 82  R 128,128
#1444 buttons 80 00 00  L 128,128  R 254,148
```

Input arrives at about 200 reports per second.

### Protocol

| Step | Channel | Data |
|---|---|---|
| Controller opens HID control | PSM `0x11` | |
| Controller opens HID interrupt | PSM `0x13` | |
| Host enables input | Control | `53 F4 42 03 00 00` |
| Host sets LED 1 | Control | `52 01 ...` (output report `0x01`) |
| Controller sends input | Interrupt | `A1 01 ...` |

### Input decoding

The three `buttons` values are bit fields. Stick values range from 0 to 255 with the centre at 128. The table follows the standard DualShock 3 layout; the log above does not verify every button.

| Bit | 1st byte | 2nd byte | 3rd byte |
|---|---|---|---|
| `01` | Select | L2 | PS |
| `02` | L3 | R2 | |
| `04` | R3 | L1 | |
| `08` | Start | R1 | |
| `10` | Up | Triangle | |
| `20` | Right | Circle | |
| `40` | Down | Cross | |
| `80` | Left | Square | |

## Conclusions and open questions

- The clone's Bluetooth radio and host-address storage work.
- The failure is on the ESP32 side. The cause (Bluetooth stack, radio or library) is not yet known.
- Possibly related, not confirmed for this controller: [ESP-IDF issue #15133](https://github.com/espressif/esp-idf/issues/15133), a ShanWan PS3 clone failing during L2CAP setup with an ESP32. The clone's chipset has not been identified.
- No result yet for [Bluepad32](https://bluepad32.readthedocs.io/en/stable/plat_arduino/) on the ESP32, or for a retry after erasing the ESP32 flash.
- To use several controllers with one host, store the same host address in each.
