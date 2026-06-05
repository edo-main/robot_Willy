# **Arduino Robot (Willy) *v4.1.0***
[Русский](README.ru.md)

### Status: 🚧 Work in progress

![From the front](assets/images/frontAI.png)
![The render](assets/images/render.png)


### This is a tracked robot platform — a semi-fun project for learning new skills through constant evolution. Most of the body parts are 3D-printed plastic. It's already the 4th version, where I stripped away unnecessary features and electronics to focus purely on the chassis and movement.

## **CONTENTS**
 - [📦**Components**](#components)
 - [🔌**Wiring diagram**](#wiring-diagram)
 - [🧩**3D models**](#3d-models)
 - [🧠**How it works**](#how-it-works)
 - [🎬**Demo**](#demo)
 - [🚀**Improvement plans**](#improvement-plans)
 - [📜**Previous versions**](#previous-versions)

## **📦COMPONENTS**  

| # | Component | Specification | Qty |
|---|-----------|---------------|-----|
| 1 | Arduino Mega | ATmega2560 | 1 |
| 2 | Gamepad | PS2 Dualshock 2.4GHz (wireless) | 1 |
| 3 | Motor Driver | BTS7960 (43A) | 2 |
| 4 | DC Motor | JGB37-555 (12V, 960 RPM) | 2 |
| 5 | OLED Display | 0.96" 128x64, I2C (4-pin) | 1 |
| 6 | Battery | LiFePO4 (12V, 20Ah) | 1 |
| 7 | Step-Down (5V) | Mini560 (Input 7-20V → Output 5V) | 1 |
| 8 | Step-Down (3.3V) | Mini560 (Input 5-20V → Output 3.3V) | 1 |
| 9 | LED Matrix | 2×2 RGB | 2 |
| 10 | High-Power LED | 1W, 3.3V | 2 |
| 11 | Relay | 5V coil | 1 |
| 12 | Battery Indicator | LiFePO4 compatible | 1 |



## **🔌WIRING DIAGRAM**

![Схема](assets/images/wiring_bb.png)

## **🧩3D MODELS**

### **Fusion 360 source files and STL files for printing:** 
## [📁 cad/](cad/)
### **Printing recommendations**
- ### Material: PETG or ABS
- ### Infill: > 30%
- ### Wall layers: > 4


## **🧠HOW IT WORKS**

**The program has two main control modes:**  
&nbsp;&nbsp;&nbsp;&nbsp;*NORMAL* - left stick controls movement  
&nbsp;&nbsp;&nbsp;&nbsp;*TANK* - L1/R1 (left/right triggers) control tracks independently  
To switch between modes: short press of the *GREEN* button (triangle)  
Both modes also have a *REVERSE* sub-mode: long press of the *RED* button (circle)  
*UP/DOWN* arrows: change speed level (1/2/3)

&nbsp;&nbsp;&nbsp;&nbsp;*--The display shows the current mode--*

**Turn lights ON/OFF: short press of the *PINK* button (square)**  
*HOLD SELECT* + *L2/R2*: change rear light color  
*HOLD SELECT* + *DOWN* arrow: hazard mode (rear red lights flashing)  
*HOLD SELECT* + *LEFT* arrow: return to normal rear light mode

## **🎬DEMO**

[Demo video] — *soon*

## **🚀IMPROVEMENT PLANS**

## Near future
- Testing and improving electronics reliability
- Motor temperature monitoring with RGB LED indication + display output
- Convenient charging connector for the battery
- USB port access for easy microcontroller connection

## Future
- Redesign chassis with simple and strong joints
- Testing and improving track durability/reliability

## Some day
- Switch to NRF wireless control
- Build a custom transmitter/receiver
- Replace Arduino Mega with a single-board computer or ESP

## **📜PREVIOUS VERSIONS**

### Earlier versions included some cool features: remote-controlled electronic track tension, automatic obstacle avoidance with ultrasonic sensors, two manual control modes, and lots of lighting options. Unfortunately, I didn't collect any media back then, so only a few low-quality photos survive.

![Earliest version](assets/images/Earliest_version.png)
![Early version 2](assets/images/Early_version_2.1.png)
![Early version 3](assets/images/Early_version_3.png)