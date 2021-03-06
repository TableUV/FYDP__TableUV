/**
 * @file motor.c
 * @author Tsugumi Murata (tmurata293)
 * @date 18 Feb 2021
 * @brief library for motor 
 *
 */

#include <avr/io.h>
#include "motor.h"
#include <util/delay.h>

void setupMotorConfig(pwm_mode_E pwm_mode){
    DDRA |= _BV(MOTOR_OUT_B);
    DDRB |= _BV(MOTOR_OUT_A); 

    // set COM (compare output mode)
    TCCR0A |= _BV(COM0A1) | _BV(COM0B1); 
        //for fast pwm: 
            // Clear (output low) on Compare Match 
            // Set (output high) at BOTTOM (non-inverting mode)
        //for phase correct
            // Clear (output low) on Compare Match when up-counting.
            // Set (output high) Compare Match when down-counting.

    if (pwm_mode == PWM_MODE_FAST){
        // set WGM (waveform generation mode)
        TCCR0A |= _BV(WGM01) | _BV(WGM00);
        // set prescaler of 8, freq = 3.91khz
        TCCR0B |=  _BV(CS01);

    }
    else if (pwm_mode == PWM_MODE_PHASE_CORRECT){
        // set WGM (waveform generation mode)
        TCCR0A |= _BV(WGM00);
        // set prescaler of 8 , freq = 1.95khz (phase correct pwm is half freq of fast pwm mode)
        TCCR0B |= _BV(CS01);

    }
}

void setMotor(motor_mode_E motor_mode, motor_pwm_duty_E percent_pwm){
    switch(motor_mode){
        case(MOTOR_MODE_COAST):
            //PORTB &= ~_BV(MOTOR_OUT_A); 
            //PORTA &= ~_BV(MOTOR_OUT_B);
            OCR0A = 0; 
            OCR0B = 0; 
        break;

        case(MOTOR_MODE_CW_COAST):
            OCR0A = percent_pwm * REG_MAX / 10 ;
            OCR0B = 0; 
            //PORTA &= ~_BV(MOTOR_OUT_B);
        break; 

        case(MOTOR_MODE_CCW_COAST):
            //PORTB &= ~_BV(MOTOR_OUT_A); 
            OCR0A = 0; 
            OCR0B = percent_pwm * REG_MAX / 10;
        break; 

        case(MOTOR_MODE_CW_BREAK):
            OCR0A = 255 ;
            OCR0B = percent_pwm * REG_MAX / 10;
        break; 

        case(MOTOR_MODE_CCW_BREAK):
            OCR0A = percent_pwm * REG_MAX / 10 ;
            OCR0B = 255 ;
        break; 

        case(MOTOR_MODE_BREAK):
            OCR0A = 255;
            OCR0B = 255;
        break; 
        default:
        break;
    }
}

void eStopMotor(){
    OCR0A = 255;
    OCR0B = 255;
}

void testMotorAll(){
    setMotor(MOTOR_MODE_COAST, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
    setMotor(MOTOR_MODE_CW_COAST, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
    setMotor(MOTOR_MODE_BREAK, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
    setMotor(MOTOR_MODE_CCW_COAST, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
    setMotor(MOTOR_MODE_CW_BREAK, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
    setMotor(MOTOR_MODE_CCW_BREAK, MOTOR_PWM_DUTY_50_PERCENT);
    _delay_ms(10);
}
