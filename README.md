# Combat Robot ESP32 Controller

Rebuilt firmware for the RC-controlled robowar bots (8kg / 15kg / 30kg classes) after the original project files were lost to a hard disk failure. Same control architecture across all three weight classes — only the drive motors, ESC current rating and battery scale up with weight.

## Architecture

- **Drive control**: RC receiver channels (steering + throttle) read via interrupt-based pulse-width capture, mixed into differential drive, output as PWM + DIR signals to the drive motor driver.
- **Weapon control**: horizontal spinner/bar driven by a brushless motor through an ESC. A dedicated RC channel arms/disarms the weapon.
- **Failsafe**: if a valid RC pulse isn't received on the steering/throttle channels within 300ms (receiver out of range, transmitter off, wiring fault), drive motors are cut and the weapon is disarmed automatically.

## Hardware

| Signal | ESP32 Pin | Notes |
| --- | --- | --- |
| Steering channel (RC in) | GPIO 34 | Input-only pin |
| Throttle channel (RC in) | GPIO 35 | Input-only pin |
| Weapon arm channel (RC in) | GPIO 32 | Input-only pin |
| Left motor PWM | GPIO 25 | To motor driver |
| Left motor DIR | GPIO 26 | To motor driver |
| Right motor PWM | GPIO 27 | To motor driver |
| Right motor DIR | GPIO 14 | To motor driver |
| Weapon ESC signal | GPIO 33 | Standard 1000-2000us servo pulse |
| Status LED | GPIO 2 | Solid = signal OK, blinking = failsafe |

Drive motor driver: any PWM+DIR style dual motor driver rated for the weight class (e.g. Cytron MDD10A for 8kg, higher-current driver for 15kg/30kg).

## Dependencies

- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) library (for weapon ESC control)

## Safety

- Weapon only arms above a clear RC pulse threshold (1700us) on a dedicated channel — never on the same channel as drive.
- Failsafe disarms the weapon and stops drive on any signal loss.
- Always test with the weapon motor disconnected first when bringing up a new build.
