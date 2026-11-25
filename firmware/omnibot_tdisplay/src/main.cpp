#include <Arduino.h>
#include <Ps3Controller.h>
#include <TFT_eSPI.h>
#include <esp_gap_bt_api.h>

#ifndef FW_VERSION
#define FW_VERSION "0.1.0"
#endif

extern "C" bool btInUse() {
  return true;
}

TFT_eSPI tft = TFT_eSPI();

volatile bool dataUpdated = false;
volatile bool stateChanged = true;
bool isConnected = false;

// Foundation for future autonomous serial override (e.g. Jetson Nano):
// When remoteOverrideActive is true (default / toggled via START), external
// serial drive commands are ignored; pressing SELECT triggers E-STOP cut-off.
enum class ControlMode : uint8_t {
  SAFE_STOP = 0,
  MANUAL_DS3 = 1,
  AUTO_SERIAL_ARMED = 2
};

ControlMode controlMode = ControlMode::MANUAL_DS3;
bool prevStart = false;
bool prevSelect = false;

unsigned long lastDraw = 0;
const unsigned long DRAW_INTERVAL_MS = 40; // ~25 FPS to prevent flicker

void notify() {
  dataUpdated = true;
}

void onConnect() {
  isConnected = true;
  stateChanged = true;
  Ps3.setPlayer(1); // Lights up LED 1 on the controller
}

void onDisconnect() {
  isConnected = false;
  controlMode = ControlMode::SAFE_STOP; // Fail-safe: cut external & manual drive on link loss
  stateChanged = true;
}

const char* getBatteryText(int bat) {
  switch (bat) {
    case ps3_status_battery_charging: return "CHG";
    case ps3_status_battery_full:     return "FULL";
    case ps3_status_battery_high:     return "HIGH";
    case ps3_status_battery_low:      return "LOW";
    case ps3_status_battery_dying:    return "DYING";
    case ps3_status_battery_shutdown: return "OFF";
    default:                          return "---";
  }
}

const char* getModeText(ControlMode mode) {
  switch (mode) {
    case ControlMode::SAFE_STOP:         return "STOP";
    case ControlMode::MANUAL_DS3:        return "MAN ";
    case ControlMode::AUTO_SERIAL_ARMED: return "AUTO";
    default:                             return "----";
  }
}

void drawStickBox(int x, int y, int size, int8_t sx, int8_t sy, const char* label) {
  // Clear interior of box
  tft.fillRect(x + 1, y + 1, size - 2, size - 2, TFT_BLACK);
  tft.drawRect(x, y, size, size, TFT_DARKGREY);

  // Center crosshair
  int cx = x + size / 2;
  int cy = y + size / 2;
  tft.drawFastHLine(x + 2, cy, size - 4, 0x2104);
  tft.drawFastVLine(cx, y + 2, size - 4, 0x2104);

  // Map -128..127 to box coordinates
  int dotX = map(sx, -128, 127, x + 4, x + size - 5);
  int dotY = map(sy, -128, 127, y + 4, y + size - 5);
  tft.fillCircle(dotX, dotY, 3, TFT_CYAN);

  // Label below box
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(label, cx, y + size + 3, 1);
}

void drawWaitingScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 22, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("OMNIBOT v" FW_VERSION, 8, 11, 2);

  tft.setTextDatum(MR_DATUM);
  tft.drawString("DS3 WAIT", 234, 11, 1);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Waiting for controller...", 120, 52, 2);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("ESP32 BT MAC (pair DS3 to this):", 120, 80, 1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String mac = Ps3.getAddress();
  mac.toUpperCase();
  tft.drawString(mac, 120, 98, 2);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Press PS button on controller", 120, 124, 1);
}

void drawConnectedLayout() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 22, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("PS3 CONNECTED", 8, 11, 2);
}

void updateControllerData() {
  // Handle START (toggle MAN <-> AUTO) and SELECT (immediate SAFE_STOP / E-STOP)
  bool curSelect = Ps3.data.button.select;
  bool curStart = Ps3.data.button.start;
  if (curSelect && !prevSelect) {
    controlMode = ControlMode::SAFE_STOP;
    Serial.println("[SAFETY] SELECT pressed -> SAFE_STOP (external serial cut off)");
  } else if (curStart && !prevStart) {
    controlMode = (controlMode == ControlMode::AUTO_SERIAL_ARMED)
                      ? ControlMode::MANUAL_DS3
                      : ControlMode::AUTO_SERIAL_ARMED;
    Serial.printf("[MODE] Switched to %s\n", getModeText(controlMode));
  }
  prevSelect = curSelect;
  prevStart = curStart;

  // Mode + Battery status in header
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(105);
  String hdrStr = String(getModeText(controlMode)) + " BAT:" + getBatteryText(Ps3.data.status.battery);
  tft.drawString(hdrStr, 234, 11, 1);

  // Analog sticks (left & right boxes)
  int8_t lx = Ps3.data.analog.stick.lx;
  int8_t ly = Ps3.data.analog.stick.ly;
  int8_t rx = Ps3.data.analog.stick.rx;
  int8_t ry = Ps3.data.analog.stick.ry;

  drawStickBox(10, 30, 56, lx, ly, "L-STICK");
  drawStickBox(76, 30, 56, rx, ry, "R-STICK");

  // Numeric stick + trigger readouts on the right side
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(96);

  char buf[24];
  snprintf(buf, sizeof(buf), "L:%4d,%4d", lx, ly);
  tft.drawString(buf, 142, 32, 1);

  snprintf(buf, sizeof(buf), "R:%4d,%4d", rx, ry);
  tft.drawString(buf, 142, 46, 1);

  uint8_t l2 = Ps3.data.analog.button.l2;
  uint8_t r2 = Ps3.data.analog.button.r2;
  snprintf(buf, sizeof(buf), "L2:%3d R2:%3d", l2, r2);
  tft.drawString(buf, 142, 64, 1);

  // Simple L2 / R2 analog bars
  int l2Bar = map(l2, 0, 255, 0, 88);
  int r2Bar = map(r2, 0, 255, 0, 88);
  tft.drawRect(142, 76, 90, 6, TFT_DARKGREY);
  tft.fillRect(143, 77, l2Bar, 4, TFT_ORANGE);
  tft.fillRect(143 + l2Bar, 77, 88 - l2Bar, 4, TFT_BLACK);

  tft.drawRect(142, 85, 90, 6, TFT_DARKGREY);
  tft.fillRect(143, 86, r2Bar, 4, TFT_ORANGE);
  tft.fillRect(143 + r2Bar, 86, 88 - r2Bar, 4, TFT_BLACK);

  // Active buttons list along the bottom
  String btns = "";
  if (Ps3.data.button.cross)    btns += "X ";
  if (Ps3.data.button.circle)   btns += "O ";
  if (Ps3.data.button.square)   btns += "SQ ";
  if (Ps3.data.button.triangle) btns += "TR ";
  if (Ps3.data.button.up)       btns += "UP ";
  if (Ps3.data.button.down)     btns += "DN ";
  if (Ps3.data.button.left)     btns += "LT ";
  if (Ps3.data.button.right)    btns += "RT ";
  if (Ps3.data.button.l1)       btns += "L1 ";
  if (Ps3.data.button.r1)       btns += "R1 ";
  if (Ps3.data.button.l3)       btns += "L3 ";
  if (Ps3.data.button.r3)       btns += "R3 ";
  if (Ps3.data.button.select)   btns += "SEL ";
  if (Ps3.data.button.start)    btns += "STA ";
  if (Ps3.data.button.ps)       btns += "PS ";
  if (btns.length() == 0)       btns = "None";

  tft.drawFastHLine(0, 105, 240, TFT_DARKGREY);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextPadding(230);
  tft.drawString("BTN: " + btns, 8, 113, 2);
}

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

void setup() {
  Serial.begin(115200);
  Serial.println("\nOmnibot T-Display Firmware v" FW_VERSION);

  tft.init();
  tft.setRotation(1); // Landscape (240x135)

  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.attachOnDisconnect(onDisconnect);
  Ps3.begin();
  esp_bt_gap_register_callback(gapLog);

  Serial.print("ESP32 BT MAC: ");
  Serial.println(Ps3.getAddress());
  Serial.println("Ready. Press the PS button on your controller.");

  drawWaitingScreen();
}

void loop() {
  if (stateChanged) {
    stateChanged = false;
    if (isConnected) {
      drawConnectedLayout();
      dataUpdated = true;
    } else {
      drawWaitingScreen();
    }
  }

  if (isConnected && dataUpdated && (millis() - lastDraw >= DRAW_INTERVAL_MS)) {
    dataUpdated = false;
    lastDraw = millis();
    updateControllerData();
  }
}
