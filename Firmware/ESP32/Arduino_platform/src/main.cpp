/**
 * @file main.c
 * 
 * @author Jianxiang (Jack) Xu
 * @author Tsugumi Murata
 * @author Jerome Villapando
 * @author Dong Jae (Alex) Park
 * 
 * @date 15 Feb 2021
 * @brief Main File
 * 
 * @license OpenSource (Non-Commercial)
 * 
 * -------------------------------------------------------------------
 * -------------------------------------------------------------------
 * --- *********************************************************** ---
 * --- * ___________     ___.   .__          ____ _______   ____ * ---
 * --- * \__    ___/____ \_ |__ |  |   ____ |    |   \   \ /   / * ---
 * --- *   |    |  \__  \ | __ \|  | _/ __ \|    |   /\   Y   /  * ---
 * --- *   |    |   / __ \| \_\ \  |_\  ___/|    |  /  \     /   * ---
 * --- *   |____|  (____  /___  /____/\___  >______/    \___/    * ---
 * --- *                \/    \/          \/                     * ---
 * --- *********************************************************** ---
 * -------------------------------------------------------------------
 * -------------------------------------------------------------------
 */

#include <Arduino.h>

// Standard libraries 
#include <string.h>
#include <stdio.h>

// TableUV Lib
#include "common.h"
#include "dev_config.h"
#include "io_ping_map.h"
#include "APP/app_slam.h"
#include "APP/app_supervisor.h"

// SDK config 
#include "sdkconfig.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/////////////////////////////////
///////   DEFINITION     ////////
/////////////////////////////////
#define ESP32_CORE_LOW_LEVEL        (0U)
#define ESP32_CORE_HIGH_LEVEL       (1U)

#define TASK_SLAM_TASK_TICK             (TASK_HZ_TO_TASK_TICK(  10/*[Hz]*/))
#define TASK_20HZ_TASK_TICK             (TASK_HZ_TO_TASK_TICK(  20/*[Hz]*/))
#define TASK_1HZ_TASK_TICK              (TASK_HZ_TO_TASK_TICK(   1/*[Hz]*/))

#define T_50MS_TASK_TICK                (MS_TO_TASK_TICK(      50/*[ms]*/))

#define MS_TO_TASK_TICK(x)              (TickType_t)((x)/(portTICK_PERIOD_MS))
#define TASK_HZ_TO_TASK_TICK(x)         (MS_TO_TASK_TICK(1000/(x))) // Range: [ < 1 kHz]

/////////////////////////////////////////
///////   PRIVATE PROTOTYPE     /////////
/////////////////////////////////////////
static void esp32_task_init(void);

static void core0_task_run50ms(void * pvParameters);
static void core0_task_run1000ms(void * pvParameters);
static void core1_task_runSLAM(void * pvParameters);
static void core1_task_runSupervisor(void * pvParameters);

///////////////////////////
///////   DATA     ////////
///////////////////////////

////////////////////////////////////////
///////   PRIVATE FUNCTION     /////////
////////////////////////////////////////
static void core0_task_run50ms(void * pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();
    for( ;; )
    {
        /* Do sth at */
        {
            //  add task
            app_supervisor_run50ms();
            dev_run50ms();
        }
        vTaskDelayUntil(&xLastWakeTime, TASK_20HZ_TASK_TICK);
    }
}
static void core0_task_run1000ms(void * pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();
    for( ;; )
    {
        /* Do sth at */
        {
            //  add task
            dev_run1000ms();
        }
        vTaskDelayUntil(&xLastWakeTime, TASK_1HZ_TASK_TICK);
    }
}

static void core1_task_runSLAM(void * pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();
    for( ;; )
    {
        /* Do sth at */
        {
            //  add task (High Level)
            app_slam_run100ms();
        }
        vTaskDelayUntil(&xLastWakeTime, TASK_SLAM_TASK_TICK);
    }
}

static void esp32_task_init()
{
    // Low Level Core Init.
    xTaskCreatePinnedToCore(
        core0_task_run50ms,     /* Function to implement the task */
        "core0_task_run50ms",   /* Name of the task */
        15000,                  /* Stack size in words */
        NULL,                   /* Task input parameter */
        1,                      /* Priority of the task */
        NULL,                   /* Task handle. */
        ESP32_CORE_LOW_LEVEL    /* Core where the task should run */
    );  
    vTaskDelay(T_50MS_TASK_TICK);

    xTaskCreatePinnedToCore(
        core0_task_run1000ms,   /* Function to implement the task */
        "core0_task_run1000ms", /* Name of the task */
        10000,                  /* Stack size in words */
        NULL,                   /* Task input parameter */
        3,                      /* Priority of the task */
        NULL,                   /* Task handle. */
        ESP32_CORE_LOW_LEVEL    /* Core where the task should run */
    );  
    vTaskDelay(T_50MS_TASK_TICK);

    //  High Level Core Init.
    xTaskCreatePinnedToCore(
        core1_task_runSLAM,    /* Function to implement the task */
        "core1_task_runSLAM",  /* Name of the task */
        20000,                  /* Stack size in words */
        NULL,                   /* Task input parameter */
        1,                      /* Priority of the task */
        NULL,                   /* Task handle. */
        ESP32_CORE_HIGH_LEVEL   /* Core where the task should run */
    );  
    vTaskDelay(T_50MS_TASK_TICK);
}

///////////////////////////////////////
///////   PUBLIC FUNCTION     /////////
///////////////////////////////////////
void setup() {
    // put your setup code here, to run once:
    // device initialization
    delay(500);
    dev_init();

    // app level init
    app_slam_init();
    app_supervisor_init();

    // esp32 task initialization
    esp32_task_init();

    // report status:
    PRINTF("[SYS] %s\n", (PROJECT_MODE_SELECTION==PROJECT_MODE_PRODUCTION) ? ("PRODUCTION"):\
        ((PROJECT_MODE_SELECTION==PROJECT_MODE_DEVELOPMENT) ? ("DEVELOPMENT"):("UNKNOWN")));
}

void loop() {
}

