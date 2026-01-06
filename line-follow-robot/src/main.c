#include "stm32f4xx.h"
#include "SSD_Array.h"
#include "configure.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "UART.h"

#define IR_SENSOR_PORT GPIOC
#define FREQUENCY 16000000UL
#define UART_PORT GPIOA
#define BTN_PIN 13
#define BTN_PORT GPIOC

volatile int stableIR = 0;         // last stable IR reading
volatile int stableCount = 0;      // how many consecutive times we've seen the same value
#define DEBOUNCE_THRESHOLD 100000      // number of consecutive reads required
volatile int counter = 0;

// trig pin = PA4, echo pin = PB0
#define TRIG_PORT GPIOA
#define TRIG_PIN 4
#define ECHO_PORT GPIOB
#define ECHO_PIN 0
void servo_angle_set(int angle);

// global variables
volatile float distance = 0.0f;
volatile int digitSelect = 0;
volatile uint32_t echo_start = 0;
volatile uint32_t echo_end = 0;
volatile bool echo_received = false;
volatile bool trigger_high = false;
volatile uint32_t currentEdge = 0;

// global variables
volatile bool move = false;
volatile int currentIR;
volatile int b3, b2, b1, b0;
volatile int left_servo_speed = 1500;  // neutral
volatile int right_servo_speed = 1500; // neutral
volatile int phase = 0;
volatile bool send_debug = false;
volatile bool button_pressed = false;
volatile uint32_t zeroIR_start = 0;      // Timestamp when 0b0000 was first seen
#define ZERO_IR_DELAY_US 500000          // 0.5 s at 1 MHz TIM5
volatile bool start_count = false;

// --------------------- Control Functions ---------------------

void servo_pwm_set(int pwm_width_left, int pwm_width_right) {
    TIM8->CCR3 = pwm_width_left; // Set pulse width for servo motor 1 (CH1)
    TIM8->CCR1 = pwm_width_right; // Set pulse width for servo motor 3 (CH3)
}


// --------------------- Interrupt Handlers ---------------------
void SysTick_Handler(void) {
    if (send_debug == false){
        send_debug = true;
    }

    TRIG_PORT->ODR |= (1 << TRIG_PIN); // Set the trigger pin high
    currentEdge = TIM5->CNT; // Get the current timer count
    while ((TIM5->CNT - currentEdge) < 10); // Wait for 10us
    TRIG_PORT->ODR &= ~(1 << TRIG_PIN); // Set the trigger pin low
}

// update SSD
void TIM2_IRQHandler(void) {
    // if (TIM2->SR & TIM_SR_UIF) {
    //     b0 = (currentIR >> 3) & 1;
    //     b1 = (currentIR >> 2) & 1;
    //     b2 = (currentIR >> 1) & 1;
    //     b3 = (currentIR >> 0) & 1;
    // int display_val = b0*1000 + b1*100 + b2*10 + b3;
    // // then in TIM2_IRQHandler:
    // SSD_update(digitSelect, display_val, 0);
    // digitSelect = (digitSelect + 1) % 4;
    // // Clear timer interrupt
    // TIM2->SR &= ~TIM_SR_UIF;
    // }
        SSD_update(digitSelect, counter/10, 2);
        digitSelect = (digitSelect + 1) % 4;
        TIM2->SR &= ~TIM_SR_UIF;
}


// button press handler
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (1 << BTN_PIN)) {
        // button_pressed = true;
        phase = 1;
        counter = 0;
        start_count = true;
        EXTI->PR |= (1 << BTN_PIN); // Clear pending bit
    }
}

void TIM4_IRQHandler(void) {
    if (TIM4->SR & TIM_SR_UIF) {
        if (start_count){
        counter++;
        }
        TIM4->SR &= ~TIM_SR_UIF; // Clear interrupt flag
    }
}

void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1 << ECHO_PIN)) {
        if (GPIOB->IDR & (1 << ECHO_PIN)) {
            // Rising edge - start measurement
            echo_start = TIM5->CNT;
        } else {
            // Falling edge - end measurement
            echo_end = TIM5->CNT;
            uint32_t pulse_width;
            
            // Handle timer overflow
            if (echo_end >= echo_start) {
                pulse_width = echo_end - echo_start;
            } else {
                pulse_width = (0xFFFFFFFF - echo_start) + echo_end;
            }
            distance = pulse_width / 148.0f;
            echo_received = true;
       }
        EXTI->PR |= (1 << ECHO_PIN); // Clear pending bit
  }
}

// --------------------- Main Program ---------------------

int main(void) {
    configure_clocks();
    configure_button();
    configure_trig_echo_button();
    PWM_Output_PC6_Init();
    configure_uart();
    SSD_init();

    configure_tim2();
    configure_tim5();  
    configure_tim4();
    SysTick_Config(FREQUENCY/2); // 0.5s SysTick


    // Configure IR sensor output/ MCU input, Set PC0–PC3 as input
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // enable GPIOC clock
    IR_SENSOR_PORT->MODER &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6));
    // for(volatile int i=0; i<1000000; i++); // Brief delay

    uart_send_string("Robot Ready. Press Button to Start.\r\n");
    // ------------------ Main loop ------------------
    // i will implement line following logic w/ a series of cases and ifs
    while(1) {
        currentIR = IR_SENSOR_PORT->IDR & 0x0F;
        // Read raw IR sensor state
        int rawIR = IR_SENSOR_PORT->IDR & 0x0F;

        // Debounce logic
        if (rawIR == stableIR) {
            stableCount++;
        } else {
            stableIR = rawIR;
            stableCount = 0;
        }

        // Only accept the value if stable long enough
        if (stableCount > DEBOUNCE_THRESHOLD) {
            currentIR = stableIR;
        }
        char buffer[50];

        if (send_debug) {
            send_debug = false;
            
            sprintf(buffer, "Phase = %d\t", phase);
            uart_send_string(buffer);
            uart_send_string("Distance: ");
            sprintf(buffer, "%.2f cm\r\t", distance);
            uart_send_string(buffer);
            uart_send_string("IR Sensors: ");
            uart_send_int32(currentIR);
            uart_send_string("\n");

        }

        // if (button_pressed){
        //     phase = 1;
        //     button_pressed = false;
        // }

        if (phase == 1){
            switch(currentIR) {
                case 0b1001: // no line detected
                    left_servo_speed = 1576;
                    right_servo_speed = 1419;
                    break;
                case 0b1111: // end line detected
                    left_servo_speed = 1500;
                    right_servo_speed = 1500;
                    phase = 2;
                    start_count = false;
                    break;
                case 0b1101: // slight right
                    left_servo_speed = 1626;
                    right_servo_speed = 1419;
                    break;
                case 0b1011: // slight left
                    left_servo_speed = 1576;
                    right_servo_speed = 1369;
                    break;
                case 0b0011: // sharp left
                    left_servo_speed = 1419;
                    right_servo_speed = 1419;
                    break;
                case 0b1100: // sharp right
                    left_servo_speed = 1576;
                    right_servo_speed = 1576;
                    break;
                case 0b0000: // all sensors on line
                    left_servo_speed = 1576;
                    right_servo_speed = 1419;
                    break;
            }  
        servo_pwm_set(left_servo_speed, right_servo_speed);
        } else if (phase == 2){
            servo_pwm_set(1526+10, 1526+10);
            // servo_pwm_set(1419, 1419)
                if (echo_received) {
                    echo_received = false;
                }
                if (distance > 100.0f && distance < 1000.0f){
                    phase = 3;
                }
        } else if (phase == 3){
            servo_pwm_set(1576, 1419); // line following speeds
            currentIR = IR_SENSOR_PORT->IDR & 0x0F;
            if (currentIR == 0b0000){
                servo_pwm_set(1500, 1500);
            } else {servo_pwm_set(1576, 1419); move = false;}
        }
    }
  return 0;
}
