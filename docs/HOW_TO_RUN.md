# How to build, test and flash

```bash
git clone https://github.com/GHOSTALERT/esp32-keypad-door-lock.git
cd esp32-keypad-door-lock
```

## 1. Host unit tests (no hardware needed)

Needs only `g++` and `make`:

```bash
make test        # 52 checks of the lock state machine
```

## 2. Build the firmware

Install [arduino-cli](https://arduino.github.io/arduino-cli/), then the ESP32 core and libraries (once):

```bash
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index && arduino-cli core install esp32:esp32
arduino-cli lib install Keypad "Adafruit SSD1306" "Adafruit GFX Library"

make compile                      # -> build/door_lock.ino.bin, .merged.bin, .elf
```

## 3. Flash the ESP32

```bash
make upload PORT=/dev/ttyUSB0     # then open a serial monitor at 115200 baud
```

Or open `firmware/door_lock/door_lock.ino` in the Arduino IDE, select **ESP32 Dev Module** and upload.

Before using it on a real door, change `LOCK_PASSWORD` in `firmware/door_lock/config.h`. Set `RELAY_ON` to `LOW` if your relay board is active-low.

## 4. Simulate in Wokwi (no hardware)

`diagram.json` wires the same circuit in [Wokwi](https://wokwi.com). A blue LED on the relay output stands in for the solenoid.

- **VS Code:** install the Wokwi extension, run `make compile`, then **Wokwi: Start Simulator** (it reads `wokwi.toml`).
- **wokwi.com:** create an ESP32 project and paste in the sketch files, `diagram.json` and `libraries.txt`.

## Wiring

| ESP32 GPIO | Connects to |
|---|---|
| 13, 12, 14, 27 | Keypad R1–R4 |
| 26, 25, 33, 32 | Keypad C1–C4 |
| 21 (SDA), 22 (SCL) | OLED SDA, SCL (VCC 3V3, GND) |
| 4 | Relay IN (relay VCC 5V, GND) |
| 18 | Green LED through 220 Ω |
| 19 | Red LED through 220 Ω |
| 23 | Buzzer + |

The relay's COM and NO switch the 12 V line to the solenoid. Put a flyback diode across the solenoid.

## Redraw the state diagram

```bash
python3 docs/make_state_diagram.py     # needs matplotlib
```
