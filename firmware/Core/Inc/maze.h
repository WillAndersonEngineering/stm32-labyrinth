//maze.h

#ifndef INC_MAZE_H_
#define INC_MAZE_H_
 
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "stm32f4xx_hal.h"
 
#define MAZE_MAX_WIDTH  15
#define MAZE_MAX_HEIGHT 15

#define USE_HAL_RNG 0

#if USE_HAL_RNG
    extern RNG_HandleTypeDef hrng;
#endif

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t cellSize;

    uint8_t hWall[MAZE_MAX_HEIGHT + 1][MAZE_MAX_WIDTH];
    uint8_t vWall[MAZE_MAX_HEIGHT][MAZE_MAX_WIDTH + 1];

    uint8_t hole[MAZE_MAX_HEIGHT][MAZE_MAX_WIDTH];
} maze_t;

void Maze_Generate(maze_t *maze, const struct config *cfg);

uint8_t Maze_HoleAt( const maze_t *maze, float x, float y, float radius);



#endif /* INC_MAZE_H_ */