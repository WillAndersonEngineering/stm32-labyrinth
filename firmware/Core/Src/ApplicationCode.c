/*
 * ApplicationCode.c
 *
 *  Created on: Nov 14, 2023
 *      Author: xcowa
 */

#include "ApplicationCode.h"

float angleX = 0;
float angleY = 0;
float physicalAngleX = 0;
float physicalAngleY = 0;
float PosX = 0;
float PosY = 0;
float VelX = 0;
float VelY = 0;
float accelX = 0;
float accelY = 0;
volatile uint8_t gameOver = 0;
volatile uint8_t gameWon = 0;
volatile uint8_t currentWaypoint = 0;
volatile uint8_t disruptorActive = 0;
volatile int32_t energyStore_mJ = 15000;
volatile uint8_t gameOverReason = 0;
uint32_t gameStartTick = 0;

maze_t g_maze;

osMutexId_t g_angle_mutex;
osMutexId_t g_pos_mutex;
osEventFlagsId_t g_game_flags;

void ApplicationInit(void)
{
	LTCD__Init();
	LTCD_Layer_Init(0);
    Gyro_Init();

	int16_t gyroNoise = Gyro_Get_Velocity_x() ^ Gyro_Get_Velocity_y();
	srand(HAL_GetTick() ^ ((uint32_t)(uint16_t)gyroNoise * 31337u));
	Maze_Generate(&g_maze, &defaultConfig);
	Display_Init();

}

void ApplicationInitTasks(void)
{
    g_angle_mutex = osMutexNew(NULL);
    g_pos_mutex = osMutexNew(NULL);
    g_game_flags = osEventFlagsNew(NULL);

	osThreadNew(GyroTask, NULL, NULL);
	osThreadNew(PhysicsTask, NULL, NULL);
    const osThreadAttr_t dispAttr = {.stack_size = 1024};
    osThreadNew(DisplayTask, NULL, &dispAttr);
    osThreadNew(LEDTask, NULL, NULL);
}

void GyroTask(void *argument){
	float angularVelocityX = 0;
	float angularVelocityY = 0;
	float dThetaX = 0;
	float dThetaY = 0;
	angleX = 0;
	angleY = 0;
	physicalAngleX = 0;
	physicalAngleY = 0;
	uint32_t lastWakeTime = osKernelGetTickCount();
	uint16_t angleGain = defaultConfig.physics.angleGain;

	while(1)
	{
		angularVelocityX = Gyro_Get_Velocity_x();
		angularVelocityY = Gyro_Get_Velocity_y();

		dThetaX = ((float)angularVelocityX * 0.0175f) * (angleGain / 1000.0f) * SAMPLE_PERIOD;
		dThetaY = ((float)angularVelocityY * 0.0175f) * (angleGain / 1000.0f) * SAMPLE_PERIOD;

		osMutexAcquire(g_angle_mutex, osWaitForever);
		angleX += dThetaX;
		angleY += dThetaY;
		physicalAngleX += angularVelocityX * GYRO_DPS_PER_LSB * SAMPLE_PERIOD;
		physicalAngleY += angularVelocityY * GYRO_DPS_PER_LSB * SAMPLE_PERIOD;
		osMutexRelease(g_angle_mutex);
		
		lastWakeTime += 20; // 50 Hz
    	osDelayUntil(lastWakeTime);
	}
}

static void Maze_Wall_Collide(void) {
    float radius = defaultConfig.drone.diameter / 2.0f;
    float cs = (float)g_maze.cellSize;
    float r2 = radius * radius;

    // HORIZONTAL WALLS
    for (uint8_t r = 0; r <= g_maze.height; r++) {
        float wy = r * cs;
        if (PosY < wy - radius || PosY > wy + radius) {
            continue;
        }
        for (uint8_t c = 0; c < g_maze.width; c++) {
            if (!g_maze.hWall[r][c]) {
                continue;
            }
            float wx0 = c * cs;
            float wx1 = (c + 1) * cs;
            if (PosX < wx0 - radius || PosX > wx1 + radius) {
                continue;
            }
            float cx;
            if (PosX < wx0) {
                cx = wx0;
            } else if (PosX > wx1) {
                cx = wx1;
            } else {
                cx = PosX;
            }
            float dx = PosX - cx;
            float dy = PosY - wy;
            if (dx*dx + dy*dy < r2) {
                if (PosY < wy) {
                    PosY = wy - radius;
                    if (VelY > 0) {
                        VelY = 0.0f;
                    }
                } else {
                    PosY = wy + radius;
                    if (VelY < 0) {
                        VelY = 0.0f;
                    }
                }
            }
        }
    }

    // VERTICAL WALLS
    for (uint8_t c = 0; c <= g_maze.width; c++) {
        float wx = c * cs;
        if (PosX < wx - radius || PosX > wx + radius) {
            continue;
        }
        for (uint8_t r = 0; r < g_maze.height; r++) {
            if (!g_maze.vWall[r][c]) {
                continue;
            }
            float wy0 = r * cs;
            float wy1 = (r + 1) * cs;
            if (PosY < wy0 - radius || PosY > wy1 + radius) {
                continue;
            }
            float cy;
            if (PosY < wy0) {
                cy = wy0;
            } else if (PosY > wy1) {
                cy = wy1;
            } else {
                cy = PosY;
            }
            float dx = PosX - wx;
            float dy = PosY - cy;
            if (dx*dx + dy*dy < r2) {
                if (PosX < wx) {
                    PosX = wx - radius;
                    if (VelX > 0) {
                        VelX = 0.0f;
                    }
                } else {
                    PosX = wx + radius;
                    if (VelX < 0) {
                        VelX = 0.0f;
                    }
                }
            }
        }
    }
}

static uint8_t Drone_In_Wall(void) {
    float radius = defaultConfig.drone.diameter / 2.0f;
    float cs = (float)g_maze.cellSize;
    float r2 = radius * radius;

    for (uint8_t r = 0; r <= g_maze.height; r++) {
        float wy = r * cs;
        if (PosY < wy - radius || PosY > wy + radius) {
            continue;
        }
        for (uint8_t c = 0; c < g_maze.width; c++) {
            if (!g_maze.hWall[r][c]) {
                continue;
            }
            float wx0 = c * cs;
            float wx1 = (c + 1) * cs;
            if (PosX < wx0 - radius || PosX > wx1 + radius) {
                continue;
            }
            float cx;
            if (PosX < wx0) {
                cx = wx0;
            } else if (PosX > wx1) {
                cx = wx1;
            } else {
                cx = PosX;
            }
            float dx = PosX - cx;
            float dy = PosY - wy;
            if (dx*dx + dy*dy < r2) {
                return 1;
            }
        }
    }
    for (uint8_t c = 0; c <= g_maze.width; c++) {
        float wx = c * cs;
        if (PosX < wx - radius || PosX > wx + radius) {
            continue;
        }
        for (uint8_t r = 0; r < g_maze.height; r++) {
            if (!g_maze.vWall[r][c]) {
                continue;
            }
            float wy0 = r * cs;
            float wy1 = (r + 1) * cs;
            if (PosY < wy0 - radius || PosY > wy1 + radius) {
                continue;
            }
            float cy;
            if (PosY < wy0) {
                cy = wy0;
            } else if (PosY > wy1) {
                cy = wy1;
            } else {
                cy = PosY;
            }
            float dx = PosX - wx;
            float dy = PosY - cy;
            if (dx*dx + dy*dy < r2) {
                return 1;
            }
        }
    }
    return 0;
}

static void Quantum_Tunnel(void) {
    float speed = sqrtf(VelX * VelX + VelY * VelY);
    if (speed < 0.1f) {
        return;
    }
    float dirX = VelX / speed;
    float dirY = VelY / speed;
    float halfDrone = defaultConfig.drone.diameter / 2.0f;
    float mazeMaxX = (float)(g_maze.width  * g_maze.cellSize) - halfDrone;
    float mazeMaxY = (float)(g_maze.height * g_maze.cellSize) - halfDrone;
    float origX = PosX;
    float origY = PosY;

    for (int i = 0; i < 200; i++) {
        PosX += dirX;
        PosY += dirY;
        if (PosX < halfDrone)
            PosX = halfDrone;
        if (PosX > mazeMaxX)
             PosX = mazeMaxX;
        if (PosY < halfDrone) 
            PosY = halfDrone;
        if (PosY > mazeMaxY)
            PosY = mazeMaxY;
        if (!Drone_In_Wall()) {
            return;
        }
    }
    PosX = origX;
    PosY = origY;
}

void PhysicsTask(void *argument){
	PosX = 90;
	PosY = 90;
	VelX = 0;
	VelY = 0;
	accelX = 0;
	accelY = 0;
	gameStartTick = osKernelGetTickCount();

    float halfDrone = defaultConfig.drone.diameter / 2.0f;
    float mazeMaxX = (float)(g_maze.width  * g_maze.cellSize) - halfDrone;
    float mazeMaxY = (float)(g_maze.height * g_maze.cellSize) - halfDrone;

	uint32_t lastWakeTime = osKernelGetTickCount();

	while(1)
	{
		if (gameOver) {
			osDelay(100);
			continue;
		}

		uint8_t btnNow = (HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN) == GPIO_PIN_SET);
		uint8_t prevDisruptorActive = disruptorActive;

		// PHYSICS UPDATE
		osMutexAcquire(g_angle_mutex, osWaitForever);
		float localAngleX = angleX;
		float localAngleY = angleY;
		float localPhysX = physicalAngleX;
		float localPhysY = physicalAngleY;
		osMutexRelease(g_angle_mutex);

		// OVER TILT
		if (localPhysX > 90.0f || localPhysX < -90.0f || localPhysY > 90.0f || localPhysY < -90.0f) {
			gameOver = 1;
			gameOverReason = REASON_TILT;
			osEventFlagsSet(g_game_flags, EVENT_GAME_OVER);
			osDelay(100);
			continue;
		}

		accelX = sinf(localAngleX * PI / 180.0f) * defaultConfig.physics.gravity;
		accelY = sinf(localAngleY * PI / 180.0f) * defaultConfig.physics.gravity;

		VelX += accelX * SAMPLE_PERIOD;
		VelY += accelY * SAMPLE_PERIOD;

		PosX += VelX * SAMPLE_PERIOD;
		PosY += VelY * SAMPLE_PERIOD;

		VelX *= 0.98f;
		VelY *= 0.98f;

		// OUT OF BOUNDS
		if (!defaultConfig.maze.hardEdged && !gameOver) {
			if (PosX < halfDrone || PosX > mazeMaxX || PosY < halfDrone || PosY > mazeMaxY) {
				gameOver = 1;
				gameOverReason = REASON_BOUNDS;
				osEventFlagsSet(g_game_flags, EVENT_GAME_OVER);
			}
		}

		osMutexAcquire(g_pos_mutex, osWaitForever);
		if (PosX < halfDrone) {
			PosX = halfDrone;
			VelX = 0.0f;
		} else if (PosX > mazeMaxX) {
			PosX = mazeMaxX;
			VelX = 0.0f;
		}
		if (PosY < halfDrone) {
			PosY = halfDrone;
			VelY = 0.0f;
		} else if (PosY > mazeMaxY) {
			PosY = mazeMaxY;
			VelY = 0.0f;
		}
		osMutexRelease(g_pos_mutex);

		// OBSTACLE CHECK
		uint8_t inWall = Drone_In_Wall();
		uint8_t overHole = Maze_HoleAt(&g_maze, PosX, PosY, defaultConfig.maze.holeDiameter / 2.0f);

		int32_t minActivation = (int32_t)defaultConfig.drone.disruptor.minActivationEnergy;
        uint8_t canBypass = 0;

        if (btnNow) {
            if (prevDisruptorActive) {
                canBypass = (energyStore_mJ > 0);
            } else {
                canBypass = (energyStore_mJ >= minActivation);
            }
        } else {
            canBypass = 0;
        }

		if (canBypass && (inWall || overHole)) {
			disruptorActive = 1;
			int32_t drain = (int32_t)((float)defaultConfig.drone.disruptor.power * SAMPLE_PERIOD);
			energyStore_mJ -= drain;
			if (energyStore_mJ < 0) {
				energyStore_mJ = 0;
			}
		} else {
			disruptorActive = 0;

			if (prevDisruptorActive && inWall) {
				Quantum_Tunnel();
				inWall = 0;
				overHole = Maze_HoleAt(&g_maze, PosX, PosY, defaultConfig.maze.holeDiameter / 2.0f);
			}

			if (inWall) {
				Maze_Wall_Collide();
			}

			if (!gameOver && overHole) {
				gameOver = 1;
				gameOverReason = REASON_HOLE;
				osEventFlagsSet(g_game_flags, EVENT_GAME_OVER);
			}

			// RECHARGE
			int32_t recharge = (int32_t)((float)defaultConfig.drone.energyStore.rechargeRate * SAMPLE_PERIOD);
			energyStore_mJ += recharge;
			if (energyStore_mJ > (int32_t)defaultConfig.drone.energyStore.maxEnergy) {
				energyStore_mJ = (int32_t)defaultConfig.drone.energyStore.maxEnergy;
			}
		}

		// TIME LIMIT
		if (!gameOver) {
			uint32_t elapsed = osKernelGetTickCount() - gameStartTick;
			if (elapsed >= defaultConfig.maze.timeToComplete) {
				gameOver = 1;
				gameOverReason = REASON_TIME;
				osEventFlagsSet(g_game_flags, EVENT_GAME_OVER);
			}
		}

		// WAYPOINT DETECTION
		if (!gameOver && currentWaypoint < defaultConfig.maze.waypoints.number) {
			float wpX = (float)defaultConfig.maze.waypoints.location[currentWaypoint].x;
			float wpY = (float)defaultConfig.maze.waypoints.location[currentWaypoint].y;
			float wpR = defaultConfig.maze.waypoints.diameter / 2.0f;
			float dx = PosX - wpX;
			float dy = PosY - wpY;
			if (dx*dx + dy*dy < wpR * wpR) {
				currentWaypoint++;
				osEventFlagsSet(g_game_flags, EVENT_WP_REACHED);
				if (currentWaypoint >= defaultConfig.maze.waypoints.number) {
					gameWon = 1;
					gameOver = 1;
					osEventFlagsSet(g_game_flags, EVENT_GAME_WIN);
				}
			}
		}

		lastWakeTime += 20; // 50 Hz
		osDelayUntil(lastWakeTime);
	}
}

void LEDTask(void *argument) {
    (void)argument;
    const uint32_t PWM_PERIOD_TICKS = 10;
    const uint32_t TICK_MS = 2;
    uint32_t pwmTick = 0;
    uint32_t redHalfTick = 0;
    uint8_t redState = 0;
    uint32_t lastWakeTime = osKernelGetTickCount();

    while (1) {
        int32_t energy = energyStore_mJ;
        int32_t maxEnergy = (int32_t)defaultConfig.drone.energyStore.maxEnergy;
        int32_t minActivation = (int32_t)defaultConfig.drone.disruptor.minActivationEnergy;
        int32_t rechargeRate = (int32_t)defaultConfig.drone.energyStore.rechargeRate;

        // GREEN LED
        uint32_t onTicks = 0;

        if (energy > 0) {
            uint32_t numerator = (uint32_t)energy * (uint32_t)PWM_PERIOD_TICKS;
            onTicks = numerator / (uint32_t)maxEnergy;
        } else {
            onTicks = 0;
        }
        if (pwmTick < onTicks) {
            HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
        }
        pwmTick++;
        if (pwmTick >= PWM_PERIOD_TICKS) {
            pwmTick = 0;
        }

        // RED LED
        if (energy >= minActivation) {
            HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
            redState = 1;
            redHalfTick = 0;
        } else {
            int32_t remaining = minActivation - energy;
            int32_t halfPeriod = (remaining * 25) / (rechargeRate * (int32_t)TICK_MS);
            if (halfPeriod < 1) {
                halfPeriod = 1;
            }
            redHalfTick++;
            if (redHalfTick >= (uint32_t)halfPeriod) {
                redState ^= 1;
                redHalfTick = 0;
                if (redState) {
                    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
                } else {
                    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
                }
            }
        }

        lastWakeTime += TICK_MS;
        osDelayUntil(lastWakeTime);
    }
}

void RunDemoForLCD(void)
{
	LCD_Clear(0,LCD_COLOR_WHITE);
	QuickDemo();
}



