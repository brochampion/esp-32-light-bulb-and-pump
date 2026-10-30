# ESP32 Relay Control

Control two bulbs and a pump from your phone using an ESP32 and a 4-channel relay module. The ESP32 creates its own Wi-Fi network and hosts a web page with animated ON/OFF cards, an All ON / All OFF buttons and an auto-off timer.

## Hardware
- ESP32 Dev Module
- 4-channel relay module (active-low)
- 2 bulbs and 1 pump, each with its own power supply wired through COM and NO

## Wiring
| Relay input | ESP32 pin | Device |
|---|---|---|
| IN1 | D13 | Bulb 1 |
| IN2 | D25 | Bulb 2 |
| IN3 | D27 | Pump |
| VCC / GND | 5V (VIN) / GND | Relay power |

## Usage
1. Open `esp32-relay-control.ino` in the Arduino IDE and select **ESP32 Dev Module**.
2. Upload the sketch.
3. Connect your phone to the Wi-Fi **smart bulb** (password `12345678`).
4. Open `http://192.168.4.1` in the browser.

## Serial Monitor (115200 baud)
The sketch logs every relay change, web request and Wi-Fi connection. You can also type:
`1`, `2`, `3` toggle a channel, `a` all ON, `o` all OFF, `s` status.

## Settings
Edit the top of the sketch: `ACTIVE_LOW`, `SELF_TEST`, `pins[]` and `names[]`.
