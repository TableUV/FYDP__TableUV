/**
 * @file dev_avr_driver.c
 * @author Tsugumi Murata (tmurata293)
 * @date 15 Feb 2021
 * @brief Device AVR Driver main file 
 *
 * This document will contains device configure content
 */

#include <Arduino.h>
#include "dev_avr_driver.h"
#include <Wire.h>
#include "../../include/avr_driver_common.h"
#include "../../include/common.h"
#include <stdbool.h>

#define I2C_RECIEVE_TIMEOUT_MILLI_SEC                                       10
#define MP_MUTEX_BLOCK_TIME_MS                                              ((1U)/portTICK_PERIOD_MS)

#define SET_MESSAGE_ESTOP_EN()                                              (1 << 13)
#define SET_MESSAGE_HAPTIC_EN()                                             (1 << 11)
#define SET_MESSAGE_TOF_CONFIG_EN(tof_sensor_config)                        (tof_sensor_config << 8)


#define RESET_MESSAGE_ESTOP_EN()                                            ~(1 << 13)
#define RESET_MESSAGE_HAPTIC_EN()                                           ~(1 << 11)
#define RESET_MESSAGE_TOF_CONFIG_EN(tof_sensor_config)                      ~(tof_sensor_config << 8)

typedef struct{
    TwoWire                     I2C;
    const uint8_t               address[NUM_AVR_DRIVER];
    const data_frame_header_E   dataFrameHeader[DATA_FRAME_HEADER_COUNT];
    uint16_t                    i2c_message[NUM_AVR_DRIVER];
    uint8_t                     reqEstop;
    uint8_t                     reqHaptic;
    tof_sensor_config_E         reqConfigTof;
    robot_motion_mode_E         reqRobotMotion;
    motor_pwm_duty_E            pwm_duty[NUM_AVR_DRIVER];
    uint16_t                    encoderCount[NUM_AVR_DRIVER];
    uint8_t                     waterLevelSig; 
    SemaphoreHandle_t           mp_mutex;
    int16_t                     l_enc_q[DEV_AVR_DRIVER_ENC_BUFFER_SIZE];
    int16_t                     r_enc_q[DEV_AVR_DRIVER_ENC_BUFFER_SIZE];
    uint8_t                     enc_buf_index;
} dev_avr_driver_data_S;

///////////////////////////
///////   DATA     ////////
///////////////////////////
static dev_avr_driver_data_S dev_avr_driver_data = {
    .I2C     = TwoWire(1),
    .address = {
        LEFT_AVR_DRIVER_I2C_ADDRESS,
        RIGHT_AVR_DRIVER_I2C_ADDRESS
    },
    .dataFrameHeader = {
        DATA_FRAME_HEADER_FIRST,
        DATA_FRAME_HEADER_SECOND,
        DATA_FRAME_HEADER_THIRD,
        DATA_FRAME_HEADER_FOURTH
    },
    .i2c_message = {
        0x0000,
        0x0000
    },
    .reqEstop   = 1,
    .reqHaptic  = 0,
    .reqConfigTof = TOF_SENSOR_CONFIG_DISABLE_ALL,
    .reqRobotMotion = ROBOT_MOTION_BREAK,
    .pwm_duty  = {
        MOTOR_PWM_DUTY_0_PERCENT, 
        MOTOR_PWM_DUTY_0_PERCENT 
    },
    .encoderCount = {
        0,
        0
    },
    .waterLevelSig = 0,
    .mp_mutex = xSemaphoreCreateBinary(),
    .l_enc_q = {0},
    .r_enc_q = {0},
    .enc_buf_index = 0
};


////////////////////////////////////////
///////   PRIVATE FUNCTION     /////////
////////////////////////////////////////

// I2C transmit two byte 
static uint8_t dev_avr_driver_transmit_two_byte(uint8_t address, uint16_t message){
	uint8_t status = 0;
    dev_avr_driver_data.I2C.beginTransmission(address);
    dev_avr_driver_data.I2C.write((message & DATA_MASK_16BIT_FIRST_8BIT) >> 8);
    dev_avr_driver_data.I2C.write( message & DATA_MASK_16BIT_SECOND_8BIT);
    status = dev_avr_driver_data.I2C.endTransmission(false);
    return status;
}

// I2C receive one byte 
static uint8_t  dev_avr_driver_receive_one_byte(uint8_t address){
    uint8_t receive_first_byte;
    dev_avr_driver_data.I2C.requestFrom(address, 1, false); 
    receive_first_byte = dev_avr_driver_data.I2C.read(); 
    return receive_first_byte;
}

// I2C receive two byte 
static uint16_t dev_avr_driver_receive_two_byte(uint8_t address){
    uint8_t buffer[2];
    int i = 0;
    dev_avr_driver_data.I2C.requestFrom(address, 2); 
    while (dev_avr_driver_data.I2C.available() && i < 2)
	{
        buffer[i] = dev_avr_driver_data.I2C.read();
		i ++;
	}
    return ((buffer[0] << 8) | buffer[1]);
}

// initialize I2C message
static inline void dev_avr_driver_init_message_two_byte(){
    dev_avr_driver_data.i2c_message[LEFT_AVR_DRIVER]   = 0b0000000001000000;
    dev_avr_driver_data.i2c_message[RIGHT_AVR_DRIVER]  = 0b0000000001000000;
}

static void avr_driver_update_i2c_message_two_byte(){  
    dev_avr_driver_init_message_two_byte();
    uint16_t temp_message_left = 0b0000000001000000, temp_message_right = 0b0000000001000000; 
    uint8_t temp_reqEstop = 0, temp_reqHaptic = 0; 
    tof_sensor_config_E temp_reqConfigTof    = TOF_SENSOR_CONFIG_DISABLE_ALL;
    robot_motion_mode_E temp_reqRobotMotion  = ROBOT_MOTION_BREAK; 
    motor_pwm_duty_E temp_left_motor_speed = MOTOR_PWM_DUTY_0_PERCENT, temp_right_motor_speed = MOTOR_PWM_DUTY_0_PERCENT; 

    temp_reqEstop  = dev_avr_driver_data.reqEstop ;
    temp_reqHaptic = dev_avr_driver_data.reqHaptic;
    temp_reqConfigTof = dev_avr_driver_data.reqConfigTof;
    temp_reqRobotMotion = dev_avr_driver_data.reqRobotMotion; 
    temp_left_motor_speed =  dev_avr_driver_data.pwm_duty[LEFT_AVR_DRIVER] ;
    temp_right_motor_speed = dev_avr_driver_data.pwm_duty[RIGHT_AVR_DRIVER];

    // req estop 
    if(temp_reqEstop){
        temp_message_left  |= SET_MESSAGE_ESTOP_EN();
        temp_message_right |= SET_MESSAGE_ESTOP_EN();
    }
    else{
        temp_message_left  &= RESET_MESSAGE_ESTOP_EN();
        temp_message_right &= RESET_MESSAGE_ESTOP_EN();
    }

    // req haptic 
    if(temp_reqHaptic){
        temp_message_right |= SET_MESSAGE_HAPTIC_EN();
    }
    else{
        temp_message_right &= RESET_MESSAGE_HAPTIC_EN();
    }

    // req ConfigTof
    if( temp_reqConfigTof ){
        temp_message_left |= SET_MESSAGE_TOF_CONFIG_EN(temp_reqConfigTof);
    }
    else{
        temp_message_left &= (~(1 << 9) & ~(1 << 8)) ;
    }

    // req RobotMotion
    if (temp_reqRobotMotion){
        switch(temp_reqRobotMotion){

            case(ROBOT_MOTION_FW_COAST):
                temp_message_left  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_REV_COAST):
                temp_message_left  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_FW_BREAK):
                temp_message_left  |= (MOTOR_COMMAND_MODE_BRAKE << 5) | (MOTOR_COMMAND_DIRECTION_CCW << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_BRAKE << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_REV_BREAK):
                temp_message_left  |= (MOTOR_COMMAND_MODE_BRAKE << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_BRAKE << 5) | (MOTOR_COMMAND_DIRECTION_CCW << 4) | (temp_right_motor_speed << 0);
                break;

            case(ROBOT_MOTION_CW_ROTATION):
                temp_message_left   |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW << 4)  | (temp_left_motor_speed << 0);
                temp_message_right  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_CCW_ROTATION):
                temp_message_left   |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW  << 4) | (temp_left_motor_speed << 0);
                temp_message_right  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW  << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_FW_DIFF_ROTATION):
                temp_message_left  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW  << 4) | (temp_right_motor_speed << 0);
            break;

            case(ROBOT_MOTION_REV_DIFF_ROTATION):
                temp_message_left  |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CW   << 4) | (temp_left_motor_speed << 0);
                temp_message_right |= (MOTOR_COMMAND_MODE_COAST << 5) | (MOTOR_COMMAND_DIRECTION_CCW  << 4) | (temp_right_motor_speed << 0);
            break;

            default:
            break; 
        }     
    }
    else{
        temp_message_left  |= SET_MESSAGE_ESTOP_EN();
        temp_message_right |= SET_MESSAGE_ESTOP_EN();
    }

    dev_avr_driver_data.i2c_message[LEFT_AVR_DRIVER]  = temp_message_left ;
    dev_avr_driver_data.i2c_message[RIGHT_AVR_DRIVER] = temp_message_right;
}

///////////////////////////////////////
///////   PUBLIC FUNCTION     /////////
///////////////////////////////////////

// init dev_avr_driver
void dev_avr_driver_init()
{
    xSemaphoreGive(dev_avr_driver_data.mp_mutex);
    dev_avr_driver_data.I2C.begin(MOTOR_I2C_SDA, MOTOR_I2C_SCL, MOTOR_I2C_FREQ); 
    dev_avr_driver_init_message_two_byte(); 
    dev_avr_driver_set_timeout(I2C_RECIEVE_TIMEOUT_MILLI_SEC); 
}

void dev_driver_avr_update20ms()
{
    uint16_t temp_left_encoder = 0, temp_right_encoder = 0;
    uint16_t temp_message_left = 0, temp_message_right = 0;
    uint8_t status = 0;
    avr_driver_update_i2c_message_two_byte(); 

    status = dev_avr_driver_transmit_two_byte( dev_avr_driver_data.address[LEFT_AVR_DRIVER] , dev_avr_driver_data.i2c_message[LEFT_AVR_DRIVER] );
    if (status == 0)
    {
        temp_left_encoder = dev_avr_driver_receive_two_byte(dev_avr_driver_data.address[LEFT_AVR_DRIVER]);
    }

    status = dev_avr_driver_transmit_two_byte( dev_avr_driver_data.address[RIGHT_AVR_DRIVER] , dev_avr_driver_data.i2c_message[RIGHT_AVR_DRIVER] );
    if (status == 0)
    {
        temp_right_encoder = dev_avr_driver_receive_two_byte(dev_avr_driver_data.address[RIGHT_AVR_DRIVER]);
    }
    //dev_avr_driver_data.waterLevelSig = dev_avr_driver_receive_one_byte(dev_avr_driver_data.address[RIGHT_AVR_DRIVER]);
    
    dev_avr_driver_data.encoderCount[LEFT_AVR_DRIVER]  = temp_left_encoder; 
    dev_avr_driver_data.encoderCount[RIGHT_AVR_DRIVER] = temp_right_encoder;

    if (xSemaphoreTake(dev_avr_driver_data.mp_mutex, MP_MUTEX_BLOCK_TIME_MS) == pdTRUE) {
        dev_avr_driver_data.l_enc_q[dev_avr_driver_data.enc_buf_index] = temp_left_encoder;
        dev_avr_driver_data.r_enc_q[dev_avr_driver_data.enc_buf_index] = temp_right_encoder;
        dev_avr_driver_data.enc_buf_index = (dev_avr_driver_data.enc_buf_index == (DEV_AVR_DRIVER_ENC_BUFFER_SIZE - 1)) ? 0 : dev_avr_driver_data.enc_buf_index + 1;
        //release the mutex 
        xSemaphoreGive(dev_avr_driver_data.mp_mutex); 
    }
#if (DEBUG_FPRINT_FEATURE_AVR_DRIVER)
    PRINTF("[ AVR:DRIVER ] left: %d, right: %d \n", dev_avr_driver_data.i2c_message[LEFT_AVR_DRIVER], dev_avr_driver_data.i2c_message[RIGHT_AVR_DRIVER]);
#endif // (DEBUG_FPRINT_FEATURE_AVR_DRIVER)
}

void dev_avr_driver_set_timeout(uint8_t milliSec){
    dev_avr_driver_data.I2C.setTimeout(milliSec); 
}

void dev_avr_driver_set_req_Estop(){
    dev_avr_driver_data.reqEstop = 1;
}

void dev_avr_driver_set_req_Haptic(){
    dev_avr_driver_data.reqHaptic = 1;
}

void dev_avr_driver_set_req_Tof_config(tof_sensor_config_E tof_sensor_config){
    dev_avr_driver_data.reqConfigTof = tof_sensor_config; 
}
void dev_avr_driver_set_req_Robot_motion(robot_motion_mode_E robot_motion_mode, motor_pwm_duty_E motor_pwm_duty_left, motor_pwm_duty_E motor_pwm_duty_right){
    dev_avr_driver_data.reqEstop = 0;
    dev_avr_driver_data.reqRobotMotion = robot_motion_mode;
    dev_avr_driver_data.pwm_duty[LEFT_AVR_DRIVER] = motor_pwm_duty_left;
    dev_avr_driver_data.pwm_duty[RIGHT_AVR_DRIVER] = motor_pwm_duty_right;
}

void dev_avr_driver_reset_req_Estop(){
    dev_avr_driver_data.reqEstop = 0;
}

void dev_avr_driver_reset_req_Haptic(){
    dev_avr_driver_data.reqHaptic = 0;
}

void dev_avr_driver_reset_req_Tof_config(){
    dev_avr_driver_data.reqConfigTof = TOF_SENSOR_CONFIG_DISABLE_ALL; 
}

void dev_avr_driver_reset_req_Robot_motion(){
    dev_avr_driver_set_req_Estop();
    dev_avr_driver_data.reqRobotMotion = ROBOT_MOTION_BREAK;
    dev_avr_driver_data.pwm_duty[LEFT_AVR_DRIVER] = MOTOR_PWM_DUTY_0_PERCENT;
    dev_avr_driver_data.pwm_duty[RIGHT_AVR_DRIVER] = MOTOR_PWM_DUTY_0_PERCENT;
}

uint16_t dev_avr_driver_get_EncoderCount(uint8_t driver_side){
    uint16_t data = 0; 
    data = dev_avr_driver_data.encoderCount[driver_side];
    return data;
}

uint8_t dev_avr_driver_get_encoder_buffers(int16_t* l_enc_buf, int16_t* r_enc_buf) {
    uint8_t buffer_size = 0;
    //take the mutex 
    if (xSemaphoreTake(dev_avr_driver_data.mp_mutex, MP_MUTEX_BLOCK_TIME_MS) == pdTRUE) {
        memcpy(l_enc_buf, dev_avr_driver_data.l_enc_q, sizeof(int16_t) * DEV_AVR_DRIVER_ENC_BUFFER_SIZE);
        memcpy(r_enc_buf, dev_avr_driver_data.r_enc_q, sizeof(int16_t) * DEV_AVR_DRIVER_ENC_BUFFER_SIZE);
        buffer_size = dev_avr_driver_data.enc_buf_index;
        dev_avr_driver_data.enc_buf_index = 0; // reset buffer index
        //release the mutex 
        xSemaphoreGive(dev_avr_driver_data.mp_mutex); 
    }
    return buffer_size;
}

uint8_t  dev_avr_driver_get_WaterLevelSig(){
    uint8_t data = 0; 
    data = dev_avr_driver_data.waterLevelSig;
    return data;
}

