# Arcade Remote

![arcade](/images/arcade_1.jpg)

## Introduction

A retro-style arcade controller for robots, built using ESP32 and Arduino.

## Electronics

| Name                       | Thumbnail                                                                | Required          | Note      |
| -------------------------- | ------------------------------------------------------------------------ | ----------------- | --------- |
| ESP32 Dev Module           | <img src="./images/esp32.jpg" alt="esp32" width="300"/>                  | 1                 |           |
| Mini 360 DC-DC step down   | <img src="./images/mini360.jpg" alt="mini360" width="300"/>              | 1                 | 5V output |
| Switch                     | <img src="./images/switch.jpg" alt="switch" width="300"/>                | 1                 |           |
| Joystick                   | <img src="./images/joystick.jpg" alt="joystick" width="300"/>            | 1                 |           |
| Push button                | <img src="./images/push_button.jpg" alt="push_button" width="300"/>      | 5                 |           |
| Red LED                    | <img src="./images/red_led.jpg" alt="red_led" width="300"/>              | 1                 |           |
| Green LED                  | <img src="./images/green_led.jpg" alt="green_led" width="300"/>          | 1                 |           |
| White LED                  | <img src="./images/white_led.jpg" alt="white_led" width="300"/>          | 4                 |           |
| 100k resistor              | <img src="./images/100k_resistor.jpg" alt="100k_resistor" width="300"/>  | 6                 |           |
| 9V battery connector       | <img src="./images/battery_connector.jpg" alt="battery_connector" width="300"/>  | 1         |           |
| 9V battery                 | <img src="./images/9v_battery.jpg" alt="9v_battery" width="300"/>        | 1                 |           |

### Connection Diagram

![diagram](./images/diagram.svg)

## 3D-Printed Parts

| Filename       | Thumbnail                                                               | Required |
| -------------- | ----------------------------------------------------------------------- | -------- |
| arcade body    | <img src="./images/arcade body.jpg" alt="arcade body" width="400"/>     | 1        |
| arcade cover   | <img src="./images/arcade cover.jpg" alt="arcade cover" width="400"/>   | 1        |
| battery cover  | <img src="./images/battery cover.jpg" alt="battery cover" width="400"/> | 1        |

## Other Parts

| Name      | Spec    | Required | Installation |
| --------- | ------- | -------- | ------------ |
| Screw     | M3 10mm | 10       | Cover and body |
| Screw     | M4 10mm | 4        | Joystick |
| Nuts      | M3      | 10       | Cover and body |
| Nuts      | M4      | 4        | Joystick |
| Standoffs | M2 Male Female Hex 4mm | 4        | Connect ESP32 to body |
| Screw     | M2 4mm  | 4        | Connect ESP32 to body |
| Magnets   | 5mm (diameter) x 2mm (thickness) | 8        | Hold battery cover |
