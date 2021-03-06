/**
 * @file collision.c
 * @author Jerome Villapando
 * @date 15 Feb 2021
 * @brief Device configure files
 *
 * This document will contains device configure content
 */

#include "collision.h"


/////////////////////////////////
///////   DEFINITION     ////////
/////////////////////////////////
#define COL_INT_ENABLED  0

typedef enum{
    BOTH_OPEN,
    LEFT_PRESSED,
    RIGHT_PRESSED,
    BOTH_PRESSED
} collision_status_t;


/////////////////////////////////////////
///////   PRIVATE PROTOTYPE     /////////
/////////////////////////////////////////
static inline void collision_private_gpio_config(void);



///////////////////////////
///////   DATA     ////////
///////////////////////////
static collision_data_S col_data;

#if (COL_INT_ENABLED)
static volatile bool col_pressed = false;
#endif //COL_INT_ENABLED



////////////////////////////////////////
///////   PRIVATE FUNCTION     /////////
////////////////////////////////////////
#if (COL_INT_ENABLED)
ISR(PCINT1_vect)
{
    cli();

    col_pressed = true;

    uint8_t pinb_reg = PINB & (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION));
    
    switch(pinb_reg)
    {
        case _BV(LEFT_COLLISION) :                          // Left collision open, right collision pressed
            col_data.col_status = RIGHT_PRESSED;
            break;
        case _BV(RIGHT_COLLISION) :                         // Right collision open, left collision pressed
            col_data.col_status = LEFT_PRESSED;
            break;
        case (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION)) : // Both switches open
            col_data.col_status = BOTH_PRESSED;
            break;
        case 0 :                                            // Both switches pressed
            col_data.col_status = BOTH_OPEN;
            break;
    }

    sei();
}
#endif //COL_INT_ENABLED
static inline void collision_private_gpio_config(void)
{
    //Set as input
    DDRB &= ~(_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION));


#if (COL_INT_ENABLED)
    // Enable Global Interrupts
    SREG |= _BV(7);
    // Enable PCINT8 to PCINT11
    GIMSK |= _BV(PCIE1);
    // Enable PCINT8 and PCINT9 for left and right collision
    PCMSK1 |= _BV(PCINT8) | _BV(PCINT9);
#endif //COL_INT_ENABLED
}

///////////////////////////////////////
///////   PUBLIC FUNCTION     /////////
///////////////////////////////////////
void collision_init(void)
{
    collision_private_gpio_config();
}

uint8_t collision_status_get(void)
{
#if (COL_INT_ENABLED)    
    return (uint8_t) col_data.col_status;
#else
    return 0;    
#endif //COL_INT_ENABLED    
}

collision_data_S collision_status_retrieve(void)
{
#if (COL_INT_ENABLED)
    uint8_t pinb_reg = PINB & (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION));
    uint8_t temp_col_status = 0;
    collision_data_S temp_struct;
    
    switch(pinb_reg)
    {
        case _BV(LEFT_COLLISION) :                          // Left collision open, right collision pressed
            temp_col_status = RIGHT_PRESSED;
            break;
        case _BV(RIGHT_COLLISION) :                         // Right collision open, left collision pressed
            temp_col_status = LEFT_PRESSED;
            break;
        case (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION)) : // Both switches open
            temp_col_status = BOTH_PRESSED;
            break;
        case 0 :                                            // Both switches pressed
            temp_col_status = BOTH_OPEN;
            break;
    }
    temp_struct.col_status = temp_col_status;
    return temp_struct;
#else
    uint8_t pinb_reg = PINB & (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION));
    switch(pinb_reg)
    {
        case _BV(LEFT_COLLISION) :                          // Left collision open, right collision pressed
            col_data.right_col_pressed = true;
            col_data.left_col_pressed = false;
            break;
        case _BV(RIGHT_COLLISION) :                         // Right collision open, left collision pressed
            col_data.right_col_pressed = false;
            col_data.left_col_pressed = true;
            break;
        case (_BV(LEFT_COLLISION) | _BV(RIGHT_COLLISION)) : // Both switches open
            col_data.right_col_pressed = false;
            col_data.left_col_pressed = false;
            break;
        case 0 :                                            // Both switches pressed
            col_data.right_col_pressed = true;
            col_data.left_col_pressed = true;            
            break;
    }
    return col_data;

#endif //COL_INT_ENABLED    
}


bool col_pressed_get(void)
{
#if (COL_INT_ENABLED)    
    cli();
    bool temp = col_pressed;
    sei();

    return temp;
#else
    return false;    
#endif //COL_INT_ENABLED   
}

void col_pressed_clear(void)
{
#if (COL_INT_ENABLED)    
    cli();
    col_pressed = false;
    sei();
#endif //COL_INT_ENABLED    
}

void collision_test(void)
{
    // collision_init();
    // DDRA |= ((PIN_DIR_OUTPUT << PA0) | (PIN_DIR_OUTPUT << PA1));
    // uint8_t temp = 0;
    
    // while(1)
    // {
    //     temp = collision_status_retrieve();
    //     switch(temp)
    //     {
    //         case 0:
    //             PORTA &= ~(1 << PA0);
    //             PORTA &= ~(1 << PA1);
    //             break;
    //         case 1:
    //             PORTA |= (1 << PA0);
    //             PORTA &= ~(1 << PA1);
    //             break;
    //         case 2:
    //             PORTA |= (1 << PA1);
    //             PORTA &= ~(1 << PA0);
    //             break;
    //         case 3:
    //             PORTA |= (1 << PA0);
    //             PORTA |= (1 << PA1);   
    //             break;         
    //     }

    //     _delay_ms(500/8);
    // }
}
