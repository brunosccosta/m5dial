# M5Stack Dial - Smart Home Controller

## Project Overview
A Home Assistant controller built on the M5Stack Dial (ESP32-S3) using LVGL for smooth animations. The dial encoder and button navigate a circular menu to control smart home devices.

## Hardware
- **Device**: M5Stack Dial (ESP32-S3)
- **Display**: 1.28" round TFT, 240×240, GC9A01 driver
- **Input**: Rotary encoder + center button
- **Connectivity**: WiFi
- **Extras**: Built-in haptic motor, RTC, microphone

## Target Entities
- 2× Air conditioners (temperature, mode)
- N× Lamps (on/off, brightness)
- Temperature sensor display (optional)

## Docs
- [Architecture & design decisions](docs/architecture.md)
- [Learnings & gotchas](docs/learnings.md)
- [Milestones & open questions](docs/milestones.md)

---
*Last updated: March 2026*
