# Wireless Page Turner (WIP)
# *work in progress, currently on research phase*

## Description
Embedded system that allows me to wirelessly turn pages.
- Design Problem: I want to be able to turn the pages of my E-book wirelessly with a handheld device.
- Solution Approach: I use a Samsung Tab S7+ for E-reading, and it has a setting where the volume buttons could be used to turn pages by pressing. I could use a microcontroller to recieve button-presses that tell a motor to either press the + button for a page turn left or the - button for a page turn right.
- Current Tech Outline
  - ESP32 to control input from peripheral, and to control motor using PWM
  - Either phone or another ESP32 to control BLE output?
