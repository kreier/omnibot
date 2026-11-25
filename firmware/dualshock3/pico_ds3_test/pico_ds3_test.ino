// DualShock 3 connection test for Raspberry Pi Pico W
// Core: Raspberry Pi Pico/RP2040 by Earle F. Philhower
// Tools -> IP/Bluetooth Stack -> "IPv4 + Bluetooth"
#include <BluetoothLock.h>
#include <btstack.h>
#include <stdarg.h>

static btstack_packet_callback_registration_t hciCallback;
static uint16_t ctrlCid = 0, intrCid = 0;
static int txStep = 0;               // 0 idle, 1 enable reports, 2 set LED 1
static volatile uint32_t reports = 0;
static volatile uint8_t last[11];

// Callbacks only write into this buffer; loop() prints it
static char logBuf[4096];
static volatile size_t logHead = 0, logTail = 0;

static void logf(const char *fmt, ...) {
  char line[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  for (char *p = line; *p; p++) {
    size_t next = (logHead + 1) % sizeof(logBuf);
    if (next == logTail) break;      // buffer full
    logBuf[logHead] = *p;
    logHead = next;
  }
}

static void sendNext() {
  // SET_REPORT feature 0xF4: tells the DS3 to start sending input
  static uint8_t enable[] = {0x53, 0xF4, 0x42, 0x03, 0x00, 0x00};
  if (txStep == 1) {
    uint8_t r = l2cap_send(ctrlCid, enable, sizeof(enable));
    logf("Enable command sent, result 0x%02X\n", r);
    txStep = 2;
    l2cap_request_can_send_now_event(ctrlCid);
  } else if (txStep == 2) {
    static uint8_t led[37] = {0x52, 0x01};   // SET_REPORT output 0x01
    led[11] = 0x02;                          // LED 1
    for (int i = 0; i < 4; i++) {
      const uint8_t t[5] = {0xFF, 0x27, 0x10, 0x00, 0x32};
      memcpy(&led[12 + 5 * i], t, 5);
    }
    uint8_t r = l2cap_send(ctrlCid, led, sizeof(led));
    logf("LED command sent, result 0x%02X\n", r);
    txStep = 0;
  }
}

static void packetHandler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
  bd_addr_t a;

  if (type == L2CAP_DATA_PACKET) {
    if (channel == intrCid && size >= 11 && packet[0] == 0xA1 && packet[1] == 0x01) {
      for (int i = 0; i < 11; i++) last[i] = packet[i];
      reports++;
    } else {
      logf("Data on cid 0x%04X, %u bytes: %02X %02X\n", channel, size,
           packet[0], size > 1 ? packet[1] : 0);
    }
    return;
  }
  if (type != HCI_EVENT_PACKET) return;

  switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
      if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        gap_local_bd_addr(a);
        logf("\nPico W Bluetooth address: %s\nWaiting. Press PS.\n", bd_addr_to_str(a));
      }
      break;

    case HCI_EVENT_CONNECTION_REQUEST:
      hci_event_connection_request_get_bd_addr(packet, a);
      logf("Connection request from %s\n", bd_addr_to_str(a));
      break;

    case HCI_EVENT_CONNECTION_COMPLETE:
      hci_event_connection_complete_get_bd_addr(packet, a);
      logf("ACL connect %s status 0x%02X\n", bd_addr_to_str(a),
           hci_event_connection_complete_get_status(packet));
      break;

    case HCI_EVENT_DISCONNECTION_COMPLETE:
      logf("ACL disconnect, reason 0x%02X\n",
           hci_event_disconnection_complete_get_reason(packet));
      ctrlCid = intrCid = 0;
      txStep = 0;
      break;

    case HCI_EVENT_PIN_CODE_REQUEST:
      hci_event_pin_code_request_get_bd_addr(packet, a);
      logf("PIN requested, answering 0000\n");
      gap_pin_code_response(a, "0000");
      break;

    case L2CAP_EVENT_INCOMING_CONNECTION: {
      uint16_t psm = l2cap_event_incoming_connection_get_psm(packet);
      uint16_t cid = l2cap_event_incoming_connection_get_local_cid(packet);
      logf("L2CAP request, PSM 0x%02X\n", psm);
      l2cap_accept_connection(cid);
      break;
    }

    case L2CAP_EVENT_CHANNEL_OPENED: {
      uint16_t psm = l2cap_event_channel_opened_get_psm(packet);
      uint8_t status = l2cap_event_channel_opened_get_status(packet);
      logf("L2CAP PSM 0x%02X opened, status 0x%02X\n", psm, status);
      if (status) break;
      if (psm == PSM_HID_CONTROL)
        ctrlCid = l2cap_event_channel_opened_get_local_cid(packet);
      if (psm == PSM_HID_INTERRUPT)
        intrCid = l2cap_event_channel_opened_get_local_cid(packet);
      if (ctrlCid && intrCid) {
        txStep = 1;
        l2cap_request_can_send_now_event(ctrlCid);
      }
      break;
    }

    case L2CAP_EVENT_CAN_SEND_NOW:
      sendNext();
      break;

    case L2CAP_EVENT_CHANNEL_CLOSED:
      logf("L2CAP channel 0x%04X closed\n",
           l2cap_event_channel_closed_get_local_cid(packet));
      break;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) delay(10);
  Serial.println("Starting Bluetooth...");

  BluetoothLock lock;
  l2cap_init();
  gap_set_security_level(LEVEL_0);
  l2cap_register_service(packetHandler, PSM_HID_CONTROL, 150, LEVEL_0);
  l2cap_register_service(packetHandler, PSM_HID_INTERRUPT, 150, LEVEL_0);

  gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
  hci_set_master_slave_policy(HCI_ROLE_MASTER);
  gap_connectable_control(1);

  hciCallback.callback = &packetHandler;
  hci_add_event_handler(&hciCallback);
  hci_power_control(HCI_POWER_ON);
}

void loop() {
  while (logTail != logHead) {
    Serial.write(logBuf[logTail]);
    logTail = (logTail + 1) % sizeof(logBuf);
  }

  static uint32_t lastPrint = 0, seen = 0;
  if (millis() - lastPrint >= 250) {
    lastPrint = millis();
    if (reports != seen) {
      seen = reports;
      Serial.printf("#%lu buttons %02X %02X %02X  L %3u,%3u  R %3u,%3u\n",
                    (unsigned long)seen, last[3], last[4], last[5],
                    last[7], last[8], last[9], last[10]);
    }
  }

  // Send 'a' in the serial monitor to show the address again
  if (Serial.read() == 'a') {
    bd_addr_t a;
    BluetoothLock lock;
    gap_local_bd_addr(a);
    Serial.printf("Pico W Bluetooth address: %s\n", bd_addr_to_str(a));
  }
}
