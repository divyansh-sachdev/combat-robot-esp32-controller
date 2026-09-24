<div align="center">

# Combat Robot ESP32 Controller

**Competition robowar firmware — differential drive mixing, weapon arming and hard failsafe**

![Domain](https://img.shields.io/badge/Domain-Real--Time_Control_Systems-00F3FF?style=for-the-badge) ![Platform](https://img.shields.io/badge/Platform-ESP32-9D00FF?style=for-the-badge) ![Classes](https://img.shields.io/badge/Classes-8_/_15_/_30_kg-0066FF?style=for-the-badge)

![ESP32](https://img.shields.io/badge/ESP32-0D1117?style=flat-square&logo=espressif&logoColor=white) ![Arduino](https://img.shields.io/badge/Arduino-0D1117?style=flat-square&logo=arduino&logoColor=white) ![C++](https://img.shields.io/badge/C++-0D1117?style=flat-square&logo=cplusplus&logoColor=white) ![RC_PWM](https://img.shields.io/badge/RC_PWM-0D1117?style=flat-square) ![Brushless_ESC](https://img.shields.io/badge/Brushless_ESC-0D1117?style=flat-square)

</div>

---

## Overview

Control firmware for the RC robowar bots campaigned across the 8&nbsp;kg, 15&nbsp;kg and 30&nbsp;kg
classes. One control architecture serves all three weight classes — only the drive motors, ESC current
rating and battery scale up with the robot.

This is combat hardware, so the engineering priority is not features but **deterministic failure
behaviour**. A 15&nbsp;kg robot with a spinning weapon that loses radio link is a safety problem, and
the failsafe path below is the part of this firmware that matters most.

Rebuilt after the original project files were lost to a hard disk failure.

## Domain &amp; Techniques

| Layer | Implementation |
| :--- | :--- |
| **Signal Capture** | Interrupt-driven pulse-width measurement of the RC receiver's steering, throttle and weapon channels — no blocking `pulseIn()` in the control loop |
| **Drive Mixing** | Steering and throttle mixed into differential left/right velocities, emitted as PWM + DIR pairs to the motor driver |
| **Weapon Arming** | The spinner ESC arms only above a 1700&nbsp;&micro;s threshold on a <em>dedicated</em> channel, never sharing a channel with drive |
| **Failsafe** | Loss of a valid RC pulse for 300&nbsp;ms — out of range, transmitter off, wiring fault — cuts drive and disarms the weapon automatically |
| **Status Feedback** | A single LED distinguishes healthy link (solid) from failsafe (blinking), readable from outside the arena |

## Pipeline

```
RC receiver
   |  steering / throttle / weapon channels
   v
[ interrupt-driven pulse width capture ]
   |
   +--> valid pulse within 300 ms? ---- NO ----> FAILSAFE
   |                                             - drive motors cut
   |                                             - weapon disarmed
   YES                                           - status LED blinks
   |
   +--> [ differential mixing ] --> L/R PWM + DIR --> motor driver
   |
   +--> [ weapon channel > 1700 us ] --> ESC arm --> spinner
```

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

Drive motor driver: any PWM+DIR style dual motor driver rated for the weight class (e.g. Cytron MDD10A
for 8kg, higher-current driver for 15kg/30kg).

## Dependencies

- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) library (for weapon ESC control)

## Safety

- Weapon only arms above a clear RC pulse threshold (1700us) on a dedicated channel — never on the
  same channel as drive.
- Failsafe disarms the weapon and stops drive on any signal loss.
- Always test with the weapon motor disconnected first when bringing up a new build.

## Repository Layout

| Path | Purpose |
| :--- | :--- |
| `CombatRobotController.ino` | Full firmware — capture, mixing, arming, failsafe state machine |

## Project Status

**Implemented:** interrupt-driven RC capture, differential drive mixing, dedicated-channel weapon
arming, 300&nbsp;ms link-loss failsafe, status indication.

**Competition record:** this control architecture placed across Robowars events at IIT Delhi, IIT
Madras, DTU, MNIT, COEP, PEC and others — see the
[full competition record](https://github.com/divyansh-sachdev).

**Roadmap:** current-sense telemetry on the weapon ESC to detect stall/jam conditions, and a
brownout-aware restart path.

---

<div align="center">
  <sub>
    Part of the <b>AI + Robotics</b> engineering portfolio of
    <a href="https://github.com/divyansh-sachdev">Divyansh Sachdev</a><br>
    90+ national &amp; international competition wins &middot; IIT / NIT / IIIT podiums
  </sub>
</div>
