/*
 *
 *    This sketch is for the WiFi joystick
 *
 *    ----------
 *    Copyright (C) 2022 - PRESENT  Zhengyu Peng
 *    Website: https://zpeng.me
 *
 *    `                      `
 *    -:.                  -#:
 *    -//:.              -###:
 *    -////:.          -#####:
 *    -/:.://:.      -###++##:
 *    ..   `://:-  -###+. :##:
 *           `:/+####+.   :##:
 *    .::::::::/+###.     :##:
 *    .////-----+##:    `:###:
 *     `-//:.   :##:  `:###/.
 *       `-//:. :##:`:###/.
 *         `-//:+######/.
 *           `-/+####/.
 *             `+##+.
 *              :##:
 *              :##:
 *              :##:
 *              :##:
 *              :##:
 *               .+:
 *
 */

#include <WiFi.h>
#include <WiFiUdp.h>

#include <Adafruit_NeoPixel.h>

// GPIO pin number for the LED
#define PIN_RGB 8

// GPIO pin number for the joystick
#define JS_UP 3
#define JS_DOWN 2
#define JS_LEFT 0
#define JS_RIGHT 1

// GPIO pin number for the buttons
#define BT_UP 14
#define BT_DOWN 15
#define BT_LEFT 18
#define BT_RIGHT 19
#define BT_SPECIAL 20


// WiFi parameters
const char *ssid = "hexapod_nougat";
const char *password = "hexapod_1234";

// UDP
WiFiUDP udp;
const IPAddress udpAddress(192, 168, 4, 1);
const int udpPort = 1234;

// Timing (ms)
const unsigned long SCAN_PERIOD_MS = 5;    // How often the switches are read
const unsigned long SETTLE_MS = 30;        // A new command must hold this long before it is sent
const unsigned long HEARTBEAT_MS = 50;     // Resend the current command at 20 Hz
const unsigned long RECONNECT_MS = 10000;  // Kick the WiFi stack if the link stays down this long
const unsigned long BLINK_MS = 250;        // Half period of the status LED blink

// Status LED brightness (0-255); full brightness is blinding through the window
const uint8_t LED_BRIGHTNESS = 40;

Adafruit_NeoPixel rgbLed(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

enum RobotCommand : uint8_t {
  CMD_STANDBY = 0,
  CMD_WALK_0 = 1,
  CMD_WALK_180 = 2,
  CMD_WALK_R45 = 3,
  CMD_WALK_R90 = 4,
  CMD_WALK_R135 = 5,
  CMD_WALK_L45 = 6,
  CMD_WALK_L90 = 7,
  CMD_WALK_L135 = 8,
  CMD_FAST_FORWARD = 9,
  CMD_FAST_BACKWARD = 10,
  CMD_TURN_LEFT = 11,
  CMD_TURN_RIGHT = 12,
  CMD_CLIMB_FORWARD = 13,
  CMD_CLIMB_BACKWARD = 14,
  CMD_ROTATE_X = 15,
  CMD_ROTATE_Y = 16,
  CMD_ROTATE_Z = 17,
  CMD_TWIST = 18
};

#pragma pack(push, 1)
struct UdpControlPacket {
  uint8_t magic;      // 0xA5
  RobotCommand cmd;
  uint32_t seq_num;
};
#pragma pack(pop)

uint32_t packet_seq_num = 0;

// Command state
RobotCommand candidate_cmd = CMD_STANDBY;  // Latest reading, still settling
RobotCommand active_cmd = CMD_STANDBY;     // Command being sent to the robot
unsigned long candidate_since_ms = 0;
unsigned long last_scan_ms = 0;
unsigned long last_send_ms = 0;

// Link state, polled from loop() so nothing is shared with the WiFi task
bool link_up = false;
bool ever_connected = false;
unsigned long last_reconnect_ms = 0;


struct RGB {
  uint8_t r, g, b;
};

constexpr RGB COLOR_OFF = { 0, 0, 0 };
constexpr RGB COLOR_RED = { 255, 0, 0 };
constexpr RGB COLOR_GREEN = { 0, 255, 0 };
constexpr RGB COLOR_BLUE = { 0, 0, 255 };
constexpr RGB COLOR_CYAN = { 0, 255, 255 };
constexpr RGB COLOR_YELLOW = { 255, 180, 0 };
constexpr RGB COLOR_MAGENTA = { 255, 0, 255 };

RGB led_color = COLOR_OFF;

void setColor(const RGB &color) {
  // Only touch the LED when the color changes; show() is not free
  if (color.r == led_color.r && color.g == led_color.g && color.b == led_color.b) {
    return;
  }
  led_color = color;
  rgbLed.setPixelColor(0, rgbLed.Color(color.r, color.g, color.b));
  rgbLed.show();
}

void setup() {
  // initilize hardware serial:
  Serial.begin(115200);
  delay(10);

  rgbLed.begin();
  rgbLed.setBrightness(LED_BRIGHTNESS);
  rgbLed.show();

  pinMode(JS_UP, INPUT_PULLUP);
  pinMode(JS_DOWN, INPUT_PULLUP);
  pinMode(JS_LEFT, INPUT_PULLUP);
  pinMode(JS_RIGHT, INPUT_PULLUP);
  pinMode(BT_UP, INPUT_PULLUP);
  pinMode(BT_DOWN, INPUT_PULLUP);
  pinMode(BT_LEFT, INPUT_PULLUP);
  pinMode(BT_RIGHT, INPUT_PULLUP);
  pinMode(BT_SPECIAL, INPUT_PULLUP);

  // Connect in the background; loop() tracks the link and keeps retrying
  WiFi.mode(WIFI_STA);
  // Modem sleep holds packets for up to a beacon interval; the remote needs low latency
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  last_reconnect_ms = millis();

  Serial.print("Connecting to ");
  Serial.println(ssid);
}


void loop() {
  const unsigned long now = millis();

  updateLink(now);

  if (now - last_scan_ms >= SCAN_PERIOD_MS) {
    last_scan_ms = now;
    scanInputs(now);
  }

  if (now - last_send_ms >= HEARTBEAT_MS) {
    sendCommand(active_cmd, now);
  }

  updateLed(now);

  delay(1);  // Yield to the WiFi task
}

/**
   Read the switches and return the command they select. The joystick takes
   priority over the buttons.
*/
RobotCommand readCommand() {
  // Switches pull to ground when pressed
  const bool js_up = digitalRead(JS_UP) == LOW;
  const bool js_down = digitalRead(JS_DOWN) == LOW;
  const bool js_left = digitalRead(JS_LEFT) == LOW;
  const bool js_right = digitalRead(JS_RIGHT) == LOW;

  const bool bt_up = digitalRead(BT_UP) == LOW;
  const bool bt_down = digitalRead(BT_DOWN) == LOW;
  const bool bt_left = digitalRead(BT_LEFT) == LOW;
  const bool bt_right = digitalRead(BT_RIGHT) == LOW;
  const bool bt_special = digitalRead(BT_SPECIAL) == LOW;

  const int js_count = js_up + js_down + js_left + js_right;

  if (js_count == 2) {
    // Diagonals
    if (js_up && js_left) return CMD_WALK_L45;
    if (js_up && js_right) return CMD_WALK_R45;
    if (js_down && js_left) return CMD_WALK_L135;
    if (js_down && js_right) return CMD_WALK_R135;
    return CMD_STANDBY;
  }

  if (js_count > 0) {
    // Holding the matching button turns a straight walk into turbo
    if (js_up) return bt_up ? CMD_FAST_FORWARD : CMD_WALK_0;
    if (js_down) return bt_down ? CMD_FAST_BACKWARD : CMD_WALK_180;
    if (js_left) return CMD_WALK_L90;
    if (js_right) return CMD_WALK_R90;
    return CMD_STANDBY;
  }

  if (bt_special) {
    if (bt_up) return CMD_ROTATE_X;
    if (bt_left) return CMD_ROTATE_Y;
    if (bt_right) return CMD_ROTATE_Z;
    if (bt_down) return CMD_TWIST;
    return CMD_STANDBY;
  }

  if (bt_up) return CMD_WALK_0;
  if (bt_down) return CMD_WALK_180;
  if (bt_left) return CMD_TURN_LEFT;
  if (bt_right) return CMD_TURN_RIGHT;

  return CMD_STANDBY;
}

/**
   Sample the inputs and switch to a new command once it has settled.

   Pressing a chord such as special + up rarely closes both switches in the
   same sample, and the robot commits to a whole gait cycle if a stray command
   lands on one of its switch points. Requiring SETTLE_MS of stability filters
   those transients (and switch bounce) out.
*/
void scanInputs(unsigned long now) {
  const RobotCommand reading = readCommand();

  if (reading != candidate_cmd) {
    candidate_cmd = reading;
    candidate_since_ms = now;
    return;
  }

  if (candidate_cmd != active_cmd && now - candidate_since_ms >= SETTLE_MS) {
    active_cmd = candidate_cmd;
    // Send right away instead of waiting for the next heartbeat
    sendCommand(active_cmd, now);
  }
}

/**
   Send one motion packet. The heartbeat timer restarts even while the link is
   down, so a dead link does not spin on send attempts.
*/
void sendCommand(RobotCommand cmd, unsigned long now) {
  last_send_ms = now;

  if (!link_up) {
    return;
  }

  UdpControlPacket packet;
  packet.magic = 0xA5;
  packet.cmd = cmd;
  packet.seq_num = packet_seq_num++;

  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t *)&packet, sizeof(packet));
  udp.endPacket();
}

/**
   Track the WiFi link and nudge the stack if it stays down.
*/
void updateLink(unsigned long now) {
  const bool up = WiFi.status() == WL_CONNECTED;

  if (up && !link_up) {
    link_up = true;
    ever_connected = true;
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    udp.begin(udpPort);
    // Tell the robot what the controls say straight away
    sendCommand(active_cmd, now);
  } else if (!up && link_up) {
    link_up = false;
    last_reconnect_ms = now;
    Serial.println("WiFi lost connection");
    udp.stop();
  }

  // Auto-reconnect normally handles this; retry by hand if it gets stuck
  if (!link_up && now - last_reconnect_ms >= RECONNECT_MS) {
    last_reconnect_ms = now;
    Serial.println("Retrying WiFi connection");
    WiFi.reconnect();
  }
}

/**
   LED color for a command category.
*/
RGB commandColor(RobotCommand cmd) {
  switch (cmd) {
    case CMD_STANDBY:
      return COLOR_GREEN;
    case CMD_FAST_FORWARD:
    case CMD_FAST_BACKWARD:
      return COLOR_YELLOW;
    case CMD_ROTATE_X:
    case CMD_ROTATE_Y:
    case CMD_ROTATE_Z:
    case CMD_TWIST:
      return COLOR_MAGENTA;
    default:
      return COLOR_CYAN;
  }
}

/**
   Blink blue while connecting, red once the link is lost, and show the
   active command's category while connected.
*/
void updateLed(unsigned long now) {
  if (link_up) {
    setColor(commandColor(active_cmd));
    return;
  }

  const bool blink_on = (now / BLINK_MS) % 2 == 0;
  if (!blink_on) {
    setColor(COLOR_OFF);
  } else {
    setColor(ever_connected ? COLOR_RED : COLOR_BLUE);
  }
}
