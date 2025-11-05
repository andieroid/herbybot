# 🌿 Herbybot: An Open-Source Robotic Platform for Hydroponic Monitoring

**Version:** Herbybot V1.0 (Design Freeze: 2025-11-17)
**License:** CERN Open Hardware Licence v2 - Permissive (OHL-P)

## 🎯 Project Overview

The Herbybot is a low-cost, miniature, open-source wheeled robot designed specifically for automated plant monitoring within standard urban hydroponic pipe systems. The platform serves as a reliable vehicle for advanced computer vision tasks, such as multi-angle image acquisition necessary for 3D plant reconstruction.

This repository provides all necessary files to **reproduce the Herbybot hardware and validate its core functional behavior (autonomous plant targeting)**, as detailed in the publication: [Insert Link to Paper 2 here once available].

---

## 🛠️ Build Status and Validation

| Component | Status | Validation Method |
| :--- | :--- | :--- |
| **Hardware Files (STLs)** | Complete (V1.0) | Verified against the final BOM. |
| **Electronics Integration**| Complete (V1.0) | Power and motor control systems are functional. |
| **Functional Test** | Benchmarking in Progress | Plant centering accuracy is being quantified (see firmware). |
| **Independent Build** | Pending (Post-Design Freeze) | Results of external build verification will be documented here. |

---

## 📋 Required Files for Replication

To build your own Herbybot, start here:

### 1. Procurement (Bill of Materials)
All commercial components, including N20 motors, DRV8833 drivers, ESP32-CAM, and power components (S2 20A controller, 18650s, USB-C socket, toggle switch), are listed here:
* [Link to file: **`/02_BOM/Herbybot_V1.0_BOM.csv`**]

### 2. Mechanical Design (3D Printing)
Download the `.STL` files for the chassis, wheel hubs, and hydroponic pipe clamps. The clamp design incorporates **MR128ZZ (8x12x3.5mm) ball bearings** to ensure smooth rail movement.
* [Link to folder: **`/01_Design_Files/`**]

### 3. Electronics and Wiring
Detailed schematics showing the full integration, including the power leads from the balanced 18650 module to the ESP32 and motor driver.
* [Link to file: **`/03_Electronics/Herbybot_V1.0_Schematic.pdf`**]

### 4. Firmware (Plant Targeting Test)
The Arduino code used for basic movement, plant detection (simple vision processing to find the plant edge), and the motor logic required to **centralize the plant** in the frame.
* [Link to folder: **`/04_Firmware/`**]

---

## 🚀 How to Build and Validate

1.  **Print Parts:** 3D print all files from the Design folder.
2.  **Assemble Hardware:** Follow the step-by-step instructions in the [Link to file: **`/05_Documentation/Assembly_Guide.pdf`**]. Pay close attention to the power module wiring and bearing installation.
3.  **Flash Firmware:** Upload the `plant_targeting_v1.0.ino` sketch to the ESP32.
4.  **Run Test:** Place the robot on the hydroponic rail next to a mature plant and activate the test mode to verify that the robot correctly detects and centrally positions the plant within a $\pm 5$ pixel tolerance.

---

## ⚖️ Licensing and Attribution

The Herbybot hardware designs and documentation are released under the **CERN Open Hardware Licence Version 2 - Permissive (OHL-P)**.

The included firmware is released under the **MIT License**.

Please cite the associated publication when using this work:

> [Citation to Paper 2: Applied Vision, Benchmarking, and Open-Source Methodology for Miniature Hydroponic Robots]
