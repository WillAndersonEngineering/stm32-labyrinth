/*
 * ApplicationCode.h
 *
 *  Created on: Nov 14, 2023
 *      Author: xcowa
 */

#ifndef INC_APPLICATIONCODE_H_
#define INC_APPLICATIONCODE_H_

#include "LCD_Driver.h"
#include "Gyro_Driver.h"
#include "display.h"
#include "config.h"
#include "maze.h"
#include "cmsis_os.h"
#include <math.h>

void ApplicationInit(void);
void ApplicationInitTasks(void);
void RunDemoForLCD(void);

void GyroTask(void *argument);
void PhysicsTask(void *argument);
void LEDTask(void *argument);

// USER DEFINITIONS
#define SAMPLE_PERIOD 0.02f // 20 ms
#define USER_BTN_PORT GPIOA
#define USER_BTN_PIN GPIO_PIN_0
#define LED_GREEN_PORT GPIOG
#define LED_GREEN_PIN GPIO_PIN_13
#define LED_RED_PORT GPIOG
#define LED_RED_PIN GPIO_PIN_14

#define HOLE_NUMBER 4
#define GYRO_DPS_PER_LSB 0.0175f
#define ANGLE_GAIN_DIVISOR 1000.0f
#define GYRO_DEADBAND 20
#define PI 3.14159f

// GYRO TEST, DELETE LATER
#define GYRO_MOTION_THRESH 100

//PHYSICS
extern float angleX;
extern float angleY;
extern float physicalAngleX;
extern float physicalAngleY;
extern float PosX;
extern float PosY;
extern float VelX;
extern float VelY;
extern float accelX;
extern float accelY;

//GAME LOGIC
extern uint32_t gameStartTick;

//MAZE
extern maze_t g_maze;

extern volatile uint8_t gameOver;
extern volatile uint8_t gameWon;
extern volatile uint8_t currentWaypoint;
extern volatile uint8_t disruptorActive;
extern volatile int32_t energyStore_mJ;
extern volatile uint8_t gameOverReason;

// RTOS SYNCHRONIZATION
extern osMutexId_t g_angle_mutex;   // GYRO WRITES, PHYSICS READS
extern osMutexId_t g_pos_mutex;     // PHYSICS WRITES, DISPLAY READS
extern osEventFlagsId_t g_game_flags;

#define EVENT_GAME_OVER (1U << 0)
#define EVENT_WP_REACHED (1U << 1)
#define EVENT_GAME_WIN (1U << 2)

#define REASON_HOLE 1
#define REASON_TIME 2
#define REASON_TILT 3
#define REASON_BOUNDS 4


#endif /* INC_APPLICATIONCODE_H_ */
