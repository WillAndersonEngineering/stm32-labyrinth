// display.h

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_
 
#include "cmsis_os.h"
#include "maze.h"

#define MAZE_ORIGIN_PX_X 30
#define MAZE_ORIGIN_PX_Y 85
#define PX_PER_MM 1

#define COLOR_BG LCD_COLOR_WHITE
#define COLOR_WALL LCD_COLOR_BLACK
#define COLOR_HOLE LCD_COLOR_RED
#define COLOR_DRONE LCD_COLOR_BLUE
#define COLOR_WP LCD_COLOR_GREEN
#define COLOR_TEXT LCD_COLOR_BLACK

#define TILT_CENTER_X 120
#define TILT_CENTER_Y 295
#define TILT_OUTER_R 18
#define TILT_DOT_R 3

void Display_Init(void);

void DisplayTask(void *argument);
 
#endif /* INC_DISPLAY_H_ */
