# M5Stack Dial - Smart Home Controller

## Project Overview
A Home Assistant controller built on the M5Stack Dial (ESP32-S3) using LVGL for smooth animations and intuitive UI. The dial encoder and button provide navigation through a menu system to control various smart home devices.

## Hardware Specifications
- **Device**: M5Stack Dial (ESP32-S3)
- **Display**: 1.28" round TFT (240x240 GC9A01)
- **Input**: Rotary encoder with center button
- **Connectivity**: WiFi (ESP32-S3)
- **Additional**: Built-in motor for haptic feedback, RTC, microphone

## Core Concept
- **Main Menu**: Navigate through different device categories using the dial
- **Sub Menus**: Each device type has its own control interface
- **Planned Controls**: 
  - Lamps (on/off, brightness, colors)
  - Air conditioning (temperature, modes)
  - Heater (temperature control)

## Design Decisions (To Be Determined)

### Input & Feedback
- [ ] Center button behavior (single press, long press, double press)
- [ ] Haptic feedback on scroll and selection
- [ ] Gesture support beyond basic dial rotation

### Home Assistant Integration
- [ ] Communication protocol (REST API, MQTT, WebSocket, ESPHome)
- [ ] Authentication method (long-lived tokens, etc.)
- [ ] WiFi configuration approach (hardcoded, web portal, BLE provisioning)
- [ ] Entity discovery (manual config vs. auto-discovery)

### UI/UX Design
- [ ] Menu layout (vertical list, circular/radial menu)
- [ ] Visual style (icons + text, text-only, icon-only)
- [ ] Animation style (smooth scrolling, fade effects, transitions)
- [ ] Color scheme and theming
- [ ] Status indicators on main screen

### Device Control Details
- [ ] **Lamps**: On/off only or brightness/color control
- [ ] **Air Conditioning**: Temperature range, mode selection, fan speeds
- [ ] **Heater**: Temperature control range and granularity
- [ ] Support for multiple devices of same type
- [ ] Control state feedback and confirmation

### System Features
- [ ] Sleep/screensaver after inactivity
- [ ] Real-time status display vs. menu-only interface
- [ ] Error handling and connection loss behavior
- [ ] OTA update support
- [ ] Configuration storage (preferences, SPIFFS, etc.)

## Development Milestones

### Milestone 1: Hardware Validation & Basic Display
**Goal**: Verify hardware works and display basic LVGL content

- Initialize M5Stack Dial hardware
- Setup LVGL with GC9A01 display driver
- Create simple "Hello World" screen
- Test rotary encoder input
- Test center button input
- Display encoder position on screen
- **Deliverable**: Working display with basic input detection

### Milestone 2: Menu Framework
**Goal**: Build the core navigation system

- Design menu data structure (categories and items)
- Implement main menu list rendering
- Add encoder navigation (scroll through items)
- Add button selection (enter/exit menus)
- Implement basic animations (scroll, transitions)
- Create placeholder sub-menu screens
- **Deliverable**: Navigable multi-level menu system

### Milestone 3: WiFi & Home Assistant Connection
**Goal**: Establish communication with Home Assistant

- WiFi connection setup
- Choose and implement HA communication protocol
- Authentication implementation
- Test basic entity state retrieval
- Test basic entity control (turn on/off)
- Error handling for connection issues
- **Deliverable**: Device connected to HA with basic control

### Milestone 4: Lamp Control Interface
**Goal**: Full implementation of first device type

- Design lamp control UI
- Implement on/off control
- Add brightness slider (if applicable)
- Add color/temperature control (if applicable)
- Show current lamp state
- Handle multiple lamp entities
- **Deliverable**: Complete lamp control functionality

### Milestone 5: Climate Control Interfaces
**Goal**: Implement air conditioning and heater controls

- Design climate control UI (shared or separate)
- Temperature setting interface
- Mode selection (if applicable)
- Display current temperature/status
- Implement control commands
- **Deliverable**: Complete climate control functionality

### Milestone 6: Polish & Advanced Features
**Goal**: Enhance UX and add nice-to-have features

- Haptic feedback integration
- Screensaver/sleep mode
- Status dashboard view
- Animation refinements
- Icon design/integration
- Error state handling improvements
- **Deliverable**: Production-ready controller

### Milestone 7: Configuration & Deployment
**Goal**: Make it easy to configure and maintain

- WiFi configuration interface
- Device entity configuration
- OTA update implementation
- Settings persistence
- Documentation for setup
- **Deliverable**: Easily deployable and configurable system

## Technical Stack

### Firmware
- **Platform**: PlatformIO (ESP32-S3)
- **Framework**: Arduino
- **UI Library**: LVGL (Light and Versatile Graphics Library)
- **Display Driver**: GC9A01
- **M5Stack Libraries**: M5Dial, M5Unified

### Home Assistant
- **Communication**: TBD (REST API, MQTT, WebSocket, or ESPHome)
- **Entities**: Light, Climate, Switch domains

## Current Status
- **Phase**: Planning & Specification
- **Next Milestone**: Milestone 1 - Hardware Validation & Basic Display

## Notes
- Keep architecture flexible for future device type additions
- Consider memory constraints of ESP32-S3
- Optimize LVGL for smooth 60fps animations on round display
- Plan for extensibility (easy to add new device types)

---
*Last Updated: March 15, 2026*
