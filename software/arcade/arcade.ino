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
#define PIN_X 3
#define PIN_Y 2
#define PIN_SW 1


// GPIO pin number for the joystick
#define JS_UP 3
#define JS_DOWN 2
#define JS_LEFT 0
#define JS_RIGHT 1

#define BT_UP 14
#define BT_DOWN 15
#define BT_LEFT 18
#define BT_RIGHT 19
#define BT_SPECIAL 20


// WiFi parameters
const char *ssid = "hexapod";
const char *password = "hexapod_1234";
boolean connected = false;

// UDP
WiFiUDP udp;
const char *udpAddress = "192.168.4.1";
const int udpPort = 1234;

int js_up = 1;
int js_down = 1;
int js_left = 1;
int js_right = 1;

int bt_up = 1;
int bt_down = 1;
int bt_left = 1;
int bt_right = 1;
int bt_special = 1;

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


struct RGB {
  uint8_t r, g, b;
};

constexpr RGB COLOR_OFF = { 0, 0, 0 };
constexpr RGB COLOR_RED = { 255, 0, 0 };
constexpr RGB COLOR_GREEN = { 0, 255, 0 };
constexpr RGB COLOR_BLUE = { 0, 0, 255 };

void setColor(const RGB &color) {
  rgbLed.setPixelColor(0, rgbLed.Color(color.r, color.g, color.b));
  rgbLed.show();
}

void setup() {
  // initilize hardware serial:
  Serial.begin(115200);
  delay(10);

  rgbLed.begin();
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


  // analogWrite(LED_1, 2);
  // analogWrite(LED_2, 2);
  // analogWrite(LED_3, 2);
  // analogWrite(LED_4, 2);

  delay(1000);

  connectToWiFi(ssid, password);
}


void loop() {
  js_up = digitalRead(JS_UP);
  js_down = digitalRead(JS_DOWN);
  js_left = digitalRead(JS_LEFT);
  js_right = digitalRead(JS_RIGHT);

  bt_up = digitalRead(BT_UP);
  bt_down = digitalRead(BT_DOWN);
  bt_left = digitalRead(BT_LEFT);
  bt_right = digitalRead(BT_RIGHT);
  bt_special = digitalRead(BT_SPECIAL);

  RobotCommand cmd_to_send = CMD_STANDBY;

  if ((js_up + js_down + js_left + js_right) != 4) {
    if ((js_up + js_down + js_left + js_right) == 2) {
      if (js_up == 0 && js_left == 0) {
        cmd_to_send = CMD_WALK_L45;
      } else if (js_up == 0 && js_right == 0) {
        cmd_to_send = CMD_WALK_R45;
      } else if (js_down == 0 && js_left == 0) {
        cmd_to_send = CMD_WALK_L135;
      } else if (js_down == 0 && js_right == 0) {
        cmd_to_send = CMD_WALK_R135;
      }
    } else {
      if (js_up == 0) {
        if (bt_up == 0) {
          cmd_to_send = CMD_FAST_FORWARD;
        } else {
          cmd_to_send = CMD_WALK_0;
        }
      } else if (js_down == 0) {
        if (bt_down == 0) {
          cmd_to_send = CMD_FAST_BACKWARD;
        } else {
          cmd_to_send = CMD_WALK_180;
        }
      } else if (js_left == 0) {
        cmd_to_send = CMD_WALK_L90;
      } else if (js_right == 0) {
        cmd_to_send = CMD_WALK_R90;
      }
    }
  } else if ((bt_up + bt_down + bt_left + bt_right + bt_special) != 5) {
    if (bt_special == 0) {
      if (bt_up == 0) {
        cmd_to_send = CMD_ROTATE_X;
      } else if (bt_left == 0) {
        cmd_to_send = CMD_ROTATE_Y;
      } else if (bt_right == 0) {
        cmd_to_send = CMD_ROTATE_Z;
      } else if (bt_down == 0) {
        cmd_to_send = CMD_TWIST;
      }
    } else if (bt_up == 0) {
      if (js_up == 0) {
        cmd_to_send = CMD_FAST_FORWARD;
      } else {
        cmd_to_send = CMD_WALK_0;
      }
    } else if (bt_down == 0) {
      if (js_down == 0) {
        cmd_to_send = CMD_FAST_BACKWARD;
      } else {
        cmd_to_send = CMD_WALK_180;
      }
    } else if (bt_left == 0) {
      cmd_to_send = CMD_TURN_LEFT;
    } else if (bt_right == 0) {
      cmd_to_send = CMD_TURN_RIGHT;
    }
  } else {
    cmd_to_send = CMD_STANDBY;
  }

  // Send packet if connected
  if (connected) {
    UdpControlPacket packet;
    packet.magic = 0xA5;
    packet.cmd = cmd_to_send;
    packet.seq_num = packet_seq_num++;

    udp.beginPacket(udpAddress, udpPort);
    udp.write((const uint8_t *)&packet, sizeof(packet));
    udp.endPacket();
  }

  delay(50); // 20Hz continuous heartbeat
}

void connectToWiFi(const char *ssid, const char *pwd) {
  // delete old config
  WiFi.disconnect(true);
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // initiate connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

// WiFi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      // when connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      // initializes the UDP state
      // this initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      setColor(COLOR_GREEN);

      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      setColor(COLOR_RED);

      break;
    default:
      break;
  }
}
