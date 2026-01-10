# Ultrasonic Radar Sweeping
## Description/Design Challenge:
Implement a sweeping object-detection radar system using an ultrasonic sensor, seven-segment-display, 180^o degree servo motor, etc. User must be able to switch between displaying inches or centimeters on button press.

*my source code can be found in /src/*
## Tech Used:
- MCU: STM32F446RE
- IDE: VSCode w/ PlatformIO
- Motors: Servo Motor, 180°
- Other: Ultrasonic Sensor, Seven-Segment Display
## New Skills Learned:
- How to implement multitasking within microcontroller, needed to handle several different tasks at once
- How ultrasonic radar works, and how to implement
- Controlling motors using pulse-width modulation, and how to implement in bare-metal
- Controlling multiple peripherals simultaneously
## Challenges & Difficulties: 
The biggest challenge from this project was getting the Ultrasonic sensor to work, which required a lot of debugging and trial-and-error.
## Result:
Success! Our system can detect nearby objects and display the distance on the display. Project recieved full credit from instructors.