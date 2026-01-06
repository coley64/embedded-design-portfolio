#ifndef CONFIGURE_H
#define CONGFIGURE_H
#include "stm32f4xx.h"  // include MCU definitions

// protytpes and definitions
void configure_clocks(void);
void configure_button(void);
void configure_tim2(void);
void configure_tim5(void);
void PWM_Output_PC6_Init(void);
void configure_tim4(void);
void configure_trig_echo_button(void);



#endif