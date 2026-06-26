# Digital Etch-A-Sketch

The **Digital Etch-A-Sketch** is a portable recreation of the classic Etch-A-Sketch drawing toy, built around an STM32 microcontroller, a 3.5" TFT display, and  rotary encoders for drawing cursor control. This project's main features are a motion-based erase and flip-book style animation playback.

> **Status:** Development on Rev1 Board  
> **Hardware Version:** Rev1

![PCB Rev1 with screen attached](./Media/Rev1/printed_name.jpeg)

---

## Features

- **Freehand Drawing**
  - Pixel-accurate cursor control
  - Encoder movement acceleration based control <span style="color:gray">(Not Yet Implemented)</span>
  - Pick-up / put-down cursor mode <span style="color:gray">(Not Yet Implemented)</span>
  - Multiple drawing thicknesses <span style="color:gray">(Not Yet Implemented)</span>

- **Physical Controls**
  - Two rotary encoders for X/Y movement

- **Shake to Erase**
  - Accelerometer detect motion for realistic screen erase gesture 

- **Portable & Low Power**
  - Powered by 2× AA batteries

---

## Bill of Materials (Summary)

| Item            | Part Number        | Qty | Notes |
|-----------------|--------------------|-----|-------|
| MCU             | STM32L433CCT6      | 1   | Main controller |
| Display         | 3.5" ILI9488 SPI   | 1   | 3.5" display |
| QSPI Flash      | For future expansion | 1   | Reserved for future use |
| 3-axis Accelerometer      | LIS3DTHR        | 1   | 3-axis velocity sensor |
| Rotary Encoder  | PEC11R-4215F-S0024 | 2   | User input |
| Battery Holder  | Adafruit 4194      | 1   | 2× AA |

---

## Hardware Subsystems

### Power Supply

- **Part:** Adafruit 4194
- **Input:** 2 × AA batteries (2.2–3.2 V)
- **Output:** 3.0 V 
- **Max Load:** ~400 mA
- **Purpose:** Powers the entire system from AA cells

---

### Microcontroller

- **Part:** STM32L433CCT3 / STM32L433CCT6  
- **Clock:** 80 MHz (internal)
- **Memory:**  
  - 256 KB Flash  
  - 64 KB RAM  

**Key Peripherals**

| Function         | Peripheral | Notes |
|------------------|------------|-------|
| Display          | SPI3       | Half-duplex, optimized for fast updates |
| Accelerometer    | SPI2, EXTI | Full-duplex |
| Flash memory     | Quad-SPI   | Memory-mapped reads |
| Rotary Encoders  | TIM1&2, EXTI | QEI mode with interrupts |
| Debug            | SWD        | DIO, CLK, NRST |

---

### Display

- **Type:** 3.5" TFT Display
- **Controller:** ILI9488
- **Interface:** Dedicated 4-wire SPI
- **Power:** 3.3 V
- **Frame Buffer:** Embedded GRAM
- **Update Modes:** Full and partial refresh
The screen is mounted to the board using the female pin headers on either side. The touchscreen functionality of the screen is not used, so the touchscreen pins are not conected.

---

### Storage

- **Part:** For future expansion
- **Capacity:** Reserved for future use
- **Interface:** Quad SPI (if added later)
- **Purpose:**
  - Potential animation frame storage
  - Potential configuration data

---

### Motion Sensor

- **Part:** LIS3DTHR
- **Interface:** SPI
- **Capabilities:** 3-axis accelerometer
- **Usage:**
  - Detect shake gesture for screen erase
  - Uses interrupts for main motion detection
  - SPI available as fallback

---

### Rotary Encoders

- **Quantity:** 2
- **Part:** PEC11R-4215F-S0024
- **Purpose:**
  - X-axis and Y-axis cursor movement
  - UI interaction via push buttons

**Connections**
- A/B channels → Timer QEI inputs
- Buttons → EXTI interrupts
- Hardware Pull-ups (open-drain behavior)

---

## Firmware Architecture

### Overview

The firmware runs mainly inside of a state machine, easily allowing for the same user inputs to be used for different purposes.

---

## Debug & Firmware Update

- **Interface:** SWD
- **Exposed Pins:**  
  - SWDIO  
  - SWCLK  
  - NRST  
  - VCC + GND  