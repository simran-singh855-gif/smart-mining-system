# ⚒️ Smart Mining & Safety System

An automated, edge-computing safety network designed for underground mining elevators (personnel cages). By processing telemetry locally in the cage, it avoids the latency and connection dropouts of sending data to a distant cloud server, ensuring instant response in high-risk environments.

This repository contains the complete firmware and configuration for the three-part safety network:
1.  **The Brain (Portenta H7):** Coordinates the network, aggregates data, and triggers emergency systems.
2.  **The Eyes (Nicla Vision):** Privacy-first TinyML camera that counts workers using coordinates.
3.  **The Nervous System (Nicla Sense ME):** Electronic nose that monitors gas, CO₂, and fire threats.

---

## ⚡ System Architecture & Hardware Roles

```mermaid
graph TD
    NV[Nicla Vision: The Eyes] -- I2C (Address 0x35) --> H7[Portenta H7: The Brain]
    NS[Nicla Sense ME: The Nervous System] -- ESLOV (I2C Host) --> H7
    
    H7 -- GPIO Output --> BUZ[Emergency Alarm Buzzer]
    H7 -- GPIO Output --> FAN[Backup Ventilation Fan Relay]
    
    subgraph Inside the Cage (Edge Nodes)
    NV
    NS
    H7
    end
```

### 1. 👁️ The Eyes (Nicla Vision)
*   **Hardware:** Arduino Nicla Vision (STM32H7, 2MP color camera).
*   **Role:** Occupancy counting and overcrowding prevention.
*   **Intelligence:** Runs an Edge Impulse **FOMO (Faster Objects, More Objects)** neural network locally to detect helmet center-points.
*   **Privacy-first:** Only outputs a count byte. Faces/video are not recorded, stored, or streamed, satisfying worker privacy rights.

### 2. 👃 The Nervous System (Nicla Sense ME)
*   **Hardware:** Arduino Nicla Sense ME (Bosch Sensortec BHI260AP, BME688, BMP390, BMM150).
*   **Role:** "Electronic nose" air quality safety monitoring.
*   **Telemetry:** Continuous measurements of compensated Temperature, Relative Humidity, Indoor Air Quality (IAQ Index), Carbon Dioxide Equivalent ($CO_2$ Eq), and Total Volatile Organic Compounds (TVOC).

### 3. 🧠 The Brain (Portenta H7)
*   **Hardware:** Arduino Portenta H7 (Dual-core ARM Cortex-M7/M4, 2MB Flash, 1MB RAM).
*   **Role:** Central controller and decision-making system.
*   **I2C Management:** Functions as the I2C master. Connects to the Nicla Sense ME via ESLOV (`Arduino_BHY2Host`) and the Nicla Vision via basic I2C (`Wire.h` at address `0x35`).
*   **Safety Logic:** Continuously evaluates sensor data against safety thresholds and instantly controls alarms and ventilation fan relays.

---

## 📁 Repository Structure

```
smart-mining-system/
├── README.md                 # Consolidated system documentation (this file)
├── brain_portenta_h7/        # Central controller code (C++)
│   ├── brain_portenta_h7.ino # Portenta H7 main Arduino sketch
│   └── config.h              # Thresholds, addresses, and pin configuration
├── eyes_nicla_vision/        # Camera person-counter code (MicroPython)
│   └── main.py               # OpenMV camera inference & I2C slave script
└── nervous_sense_me/         # Environmental sensor code (C++)
    └── nervous_sense_me.ino  # Nicla Sense ME sensor slave bridge
```

---

## 🛠️ Hardware Wiring & Pins

Connecting the boards inside the elevator cage is straightforward using the standard Pro ESLOV connectors.

| Connection | Portenta H7 / Carrier | Nicla Sense ME | Nicla Vision |
| :--- | :--- | :--- | :--- |
| **ESLOV (I2C + Power)** | ESLOV Port (5-pin) | ESLOV Port (5-pin) | — |
| **Standard I2C** | SDA (SDA Pin) / SCL (SCL Pin) | — | SDA / SCL Pins |
| **Common GND** | GND Pin | — | GND Pin |
| **Buzzer Actuator** | `D2` (Digital Out) | — | — |
| **Ventilation Relay** | `D3` (Digital Out) | — | — |

> [!IMPORTANT]
> When wiring the Nicla Vision to the Portenta H7 over standard I2C without an ESLOV board, you **must** place pull-up resistors (4.7kΩ recommended) between SDA/SCL lines and the 3.3V power rail.

---

## 🚨 System Logic & Safety Thresholds

All safety thresholds are declared in [brain_portenta_h7/config.h](file:///c:/Users/Simran%20Singh/OneDrive/Desktop/Smart%20Minning%20System/brain_portenta_h7/config.h):

| Condition | Threshold | System Action | indicator |
| :--- | :--- | :--- | :--- |
| **Worker Overload** | $> 8$ passengers | Sound Constant Buzzer, Turn On Backup Fan | Red LED ON |
| **Toxic Air ($CO_2$)** | $> 1000$ ppm | Sound Pulsed Buzzer, Turn On Backup Fan | Green LED ON |
| **Toxic Gas (TVOC / IAQ)**| $> 500$ ppb / $> 200$ IAQ | Sound Pulsed Buzzer, Turn On Backup Fan | Green LED ON |
| **Fire / Heat Spike** | $> 40.0^\circ\text{C}$ | Sound Pulsed Buzzer, Turn On Backup Fan | Red LED ON |
| **Normal Operations** | All parameters safe | Actuators Idle | Blue status LED pulsing |

---

## 🚀 Step-by-Step Installation & Deployment Guide

### Step 1: Program the Nicla Sense ME (Environmental Sensor)
The Nicla Sense ME acts as the environmental sensor daemon, updating register data for the Portenta H7 host.

1.  **Configure Arduino IDE:**
    *   Open the Arduino IDE. Go to **Tools > Board > Boards Manager...**
    *   Search for **`Arduino Mbed OS Nicla Boards`** and click **Install**.
    *   Go to **Tools > Manage Libraries...**, search for **`Arduino_BHY2`**, and install the library.
2.  **Upload Firmware:**
    *   Connect the Nicla Sense ME to your PC via Micro-USB.
    *   Select **Tools > Board > Arduino Mbed OS Nicla Boards > Arduino Nicla Sense ME**.
    *   Open [nervous_sense_me.ino](file:///c:/Users/Simran%20Singh/OneDrive/Desktop/Smart%20Minning%20System/nervous_sense_me/nervous_sense_me.ino) and upload it.
3.  **Sensor Calibration:**
    *   The BME688 air sensor utilizes a heated micro-plate. Upon initial boot, it will report an Indoor Air Quality (IAQ) accuracy of `0`.
    *   Leave the system running in fresh air for 20-30 minutes. Once BSEC starts reporting accuracy `1`, `2`, or `3`, the baseline has calibrated.

---

### Step 2: Deploy the FOMO Model to Nicla Vision (Camera Node)
The camera processes camera frames locally using MicroPython, counting workers and updating a custom I2C register.

1.  **Set Up OpenMV IDE & Firmware:**
    *   Download and install the [OpenMV IDE](https://openmv.io/pages/download).
    *   Connect the Nicla Vision to your PC via Micro-USB. Connect using the OpenMV connect icon (plug symbol) and update the firmware if prompted.
2.  **Train & Download FOMO Model:**
    *   Create a free account on [Edge Impulse Studio](https://studio.edgeimpulse.com/).
    *   Create an Object Detection project and capture images of workers/helmets. Bounding-box label the heads as `person`.
    *   Choose **FOMO (MobileNet V2 0.35)** as your model. Set the input size to `240x240`. Train the model.
    *   Go to **Deployment**, select **TensorFlow Lite (int8 or float32)**, and build.
    *   Rename the downloaded model file to **`model.tflite`**.
3.  **Flash the Camera:**
    *   Mount the Nicla Vision on your PC. It will show up as a USB storage drive.
    *   Copy **`model.tflite`** directly to the root folder of this drive.
    *   Open [main.py](file:///c:/Users/Simran%20Singh/OneDrive/Desktop/Smart%20Minning%20System/eyes_nicla_vision/main.py) in the OpenMV IDE. 
    *   Click **File > Save open script to board (as main.py)**. The camera will now execute the people-counter script automatically on power-up.

---

### Step 3: Program the Portenta H7 (The Brain)
The Portenta H7 connects to both sensors and manages decision-making.

1.  **Configure Arduino IDE:**
    *   Go to **Tools > Board > Boards Manager...**
    *   Search for **`Arduino Mbed OS Boards (M7 Core)`** and install the core.
    *   Go to **Tools > Manage Libraries...** and install **`Arduino_BHY2Host`**.
2.  **Verify Thresholds:**
    *   Check [config.h](file:///c:/Users/Simran%20Singh/OneDrive/Desktop/Smart%20Minning%20System/brain_portenta_h7/config.h) to adjust values like `MAX_WORKERS` (default `8`) or gas limits.
3.  **Upload & Monitor:**
    *   Select **Tools > Board > Arduino Mbed OS Boards (M7 Core) > Arduino Portenta H7 (M7 Core)**.
    *   Connect the Portenta H7 to your PC, open [brain_portenta_h7.ino](file:///c:/Users/Simran%20Singh/OneDrive/Desktop/Smart%20Minning%20System/brain_portenta_h7/brain_portenta_h7.ino), and upload it.
    *   Open the Serial Monitor at **115200 baud** to view system telemetry logs.

---

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
