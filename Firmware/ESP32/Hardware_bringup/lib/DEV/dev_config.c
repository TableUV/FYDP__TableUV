/**
 * @file dev_config.c
 * @author Jianxiang (Jack) Xu
 * @date 15 Feb 2021
 * @brief Device configure files
 *
 * This document will contains device configure content
 */

#include "dev_config.h"

// TableUV Lib
#include "dev_avr_actuator.h"
#include "dev_avr_sensor.h"
#include "dev_battery.h"
#include "dev_config.h"
#include "dev_led.h"
#include "dev_ToF_Lidar.h"
#include "dev_uv.h"
#include "dev_vl53l1x.h"

/////////////////////////////////
///////   DEFINITION     ////////
/////////////////////////////////


/////////////////////////////////////////
///////   PRIVATE PROTOTYPE     /////////
/////////////////////////////////////////


///////////////////////////
///////   DATA     ////////
///////////////////////////


////////////////////////////////////////
///////   PRIVATE FUNCTION     /////////
////////////////////////////////////////


///////////////////////////////////////
///////   PUBLIC FUNCTION     /////////
///////////////////////////////////////
void dev_init(void)
{
    // sub-device  initialization
    dev_avr_actuator_init();
    dev_avr_sensor_init();
    dev_battery_init();
    dev_led_init();
    dev_ToF_Lidar_init();
    dev_uv_init();
    dev_vl53l1x_init();
}

void dev_run10ms(void)
{
    // Do  nothing
}

void dev_run100ms(void)
{
    // Do  nothing
}

void dev_run1000ms(void)
{
    // Do  nothing
}

