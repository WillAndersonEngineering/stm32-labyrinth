// display.c

#include "display.h"
#include "LCD_Driver.h"
#include "ApplicationCode.h"
#include "config.h"
#include <math.h>

extern void LCD_Draw_Pixel(uint16_t x, uint16_t y, uint16_t color);
extern uint16_t frameBuffer[LCD_PIXEL_WIDTH * LCD_PIXEL_HEIGHT];

static inline void safe_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x < LCD_PIXEL_WIDTH && y < LCD_PIXEL_HEIGHT) {
        if (frameBuffer[y * LCD_PIXEL_WIDTH + x] != COLOR_WALL) {
            LCD_Draw_Pixel(x, y, color);
        }
    }
}

static inline uint16_t mm2px_x(float x_mm) {
    return (uint16_t)(MAZE_ORIGIN_PX_X + x_mm * PX_PER_MM);
}
static inline uint16_t mm2px_y(float y_mm) {
    return (uint16_t)(MAZE_ORIGIN_PX_Y + y_mm * PX_PER_MM);
}

static void draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color) {
    for (uint16_t i = 0; i < len; i++) {
        LCD_Draw_Pixel(x + i, y, color);
    }
}

static void draw_circle_outline(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = r, y = 0;
    int16_t err = 1 - r;
    while (x >= y) {
        safe_pixel((uint16_t)(cx + x), (uint16_t)(cy + y), color);
        safe_pixel((uint16_t)(cx + y), (uint16_t)(cy + x), color);
        safe_pixel((uint16_t)(cx - y), (uint16_t)(cy + x), color);
        safe_pixel((uint16_t)(cx - x), (uint16_t)(cy + y), color);
        safe_pixel((uint16_t)(cx - x), (uint16_t)(cy - y), color);
        safe_pixel((uint16_t)(cx - y), (uint16_t)(cy - x), color);
        safe_pixel((uint16_t)(cx + y), (uint16_t)(cy - x), color);
        safe_pixel((uint16_t)(cx + x), (uint16_t)(cy - y), color);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}



static void draw_tilt_ring(void) {
    int16_t cx = TILT_CENTER_X;
    int16_t cy = TILT_CENTER_Y;
    int16_t x = TILT_OUTER_R;
    int16_t y = 0;
    int16_t err = 1 - TILT_OUTER_R;
    while (x >= y) {
        LCD_Draw_Pixel((uint16_t)(cx+x), (uint16_t)(cy+y), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx+y), (uint16_t)(cy+x), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx-y), (uint16_t)(cy+x), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx-x), (uint16_t)(cy+y), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx-x), (uint16_t)(cy-y), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx-y), (uint16_t)(cy-x), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx+y), (uint16_t)(cy-x), COLOR_TEXT);
        LCD_Draw_Pixel((uint16_t)(cx+x), (uint16_t)(cy-y), COLOR_TEXT);
        y++;
        if (err < 0) { err += 2*y+1; }
        else { x--; err += 2*(y-x)+1; }
    }
    LCD_Draw_Pixel((uint16_t)cx, (uint16_t)cy, COLOR_TEXT);
}

static void draw_tilt_dot(float ax, float ay, uint16_t color) {
    const float DEADZONE = 5.0f;
    int16_t inner_r = TILT_OUTER_R - TILT_DOT_R - 1;
    int16_t dotX, dotY;

    float mag = sqrtf(ax*ax + ay*ay);
    if (mag < DEADZONE) {
        dotX = TILT_CENTER_X;
        dotY = TILT_CENTER_Y;
    } else {
        float snap_rad = roundf(atan2f(ay, ax) / (PI / 4.0f)) * (PI / 4.0f);
        dotX = TILT_CENTER_X + (int16_t)((float)inner_r * cosf(snap_rad));
        dotY = TILT_CENTER_Y + (int16_t)((float)inner_r * sinf(snap_rad));
    }

    int16_t r = TILT_DOT_R;
    for (int16_t dy = -r; dy <= r; dy++) {
        for (int16_t dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                uint16_t px = (uint16_t)(dotX + dx);
                uint16_t py = (uint16_t)(dotY + dy);
                if (px < LCD_PIXEL_WIDTH && py < LCD_PIXEL_HEIGHT) {
                    LCD_Draw_Pixel(px, py, color);
                }
            }
        }
    }
}

static void draw_maze(const maze_t *maze) {

    uint16_t cs_px = maze->cellSize * PX_PER_MM;

    for (uint8_t r = 0; r <= maze->height; r++) {
        for (uint8_t c = 0; c < maze->width; c++){
            if (maze->hWall[r][c]){
                uint16_t wy = mm2px_y(r * maze->cellSize);
                uint16_t wx = mm2px_x(c * maze->cellSize);
                draw_hline(wx, wy, cs_px, COLOR_WALL);
                draw_hline(wx, wy + 1, cs_px, COLOR_WALL);
            }
        }
    }

    for (uint8_t c = 0; c <= maze->width; c++) {
        for (uint8_t r = 0; r < maze->height; r++){
            if (maze->vWall[r][c]){
                uint16_t vx = mm2px_x(c * maze->cellSize);
                uint16_t vy = mm2px_y(r * maze->cellSize);
                for (uint16_t i = 0; i < cs_px; i++) {
                    LCD_Draw_Pixel(vx, vy + i, COLOR_WALL);
                    LCD_Draw_Pixel(vx + 1, vy + i, COLOR_WALL);
                }
            }
        }
    }

    uint16_t hole_r = (uint16_t)(defaultConfig.maze.holeDiameter * PX_PER_MM / 2);
    for (uint8_t r = 0; r < maze->height; r++) {
        for (uint8_t c = 0; c < maze->width; c++) {
            if (maze->hole[r][c]) {
                uint16_t hole_cx = mm2px_x((c + 0.5f) * maze->cellSize);
                uint16_t hole_cy = mm2px_y((r + 0.5f) * maze->cellSize);
                LCD_Draw_Circle_Fill(hole_cx, hole_cy, hole_r, COLOR_HOLE);
            }
        }
    }
}

void draw_drone(float x_mm, float y_mm) {
    uint16_t r_px = (uint16_t)(defaultConfig.drone.diameter / 2);
    int16_t cx = (int16_t)mm2px_x(x_mm);
    int16_t cy = (int16_t)mm2px_y(y_mm);
    for (int16_t dy = -(int16_t)r_px; dy <= (int16_t)r_px; dy++) {
        for (int16_t dx = -(int16_t)r_px; dx <= (int16_t)r_px; dx++) {
            if (dx*dx + dy*dy <= (int16_t)(r_px * r_px)) {
                uint16_t px = (uint16_t)(cx + dx);
                uint16_t py = (uint16_t)(cy + dy);
                if (px < LCD_PIXEL_WIDTH && py < LCD_PIXEL_HEIGHT) {
                    if (frameBuffer[py * LCD_PIXEL_WIDTH + px] != COLOR_WALL) {
                        LCD_Draw_Pixel(px, py, COLOR_DRONE);
                    }
                }
            }
        }
    }
}

static void draw_waypoint_circle(uint8_t index, uint16_t color) {
    if (index >= defaultConfig.maze.waypoints.number) {
        return;
    }
    float wx = (float)defaultConfig.maze.waypoints.location[index].x;
    float wy = (float)defaultConfig.maze.waypoints.location[index].y;
    int16_t r_px = (int16_t)(defaultConfig.maze.waypoints.diameter / 2);
    draw_circle_outline(mm2px_x(wx), mm2px_y(wy), r_px, color);
}

static void draw_status(uint8_t localOver, uint8_t localWon, uint8_t localReason) {
    char buf[32];
    uint32_t elapsed_ms;
    uint32_t remaining_ms;
    uint32_t seconds;
    uint32_t tenths;

    LCD_SetTextColor(COLOR_TEXT);

    if (localWon) {
        LCD_DisplayString(50, 10, (uint8_t*)"YOU WIN!");
    } else if (localOver) {
        LCD_DisplayString(50, 10, (uint8_t*)"GAME OVER");
        if (localReason == REASON_HOLE) {
            LCD_DisplayString(10, 38, (uint8_t*)"Fell in a hole");
        } else if (localReason == REASON_TIME) {
            LCD_DisplayString(10, 38, (uint8_t*)"Time expired");
        } else if (localReason == REASON_TILT) {
            LCD_DisplayString(10, 38, (uint8_t*)"Over tilted");
        } else if (localReason == REASON_BOUNDS) {
            LCD_DisplayString(10, 38, (uint8_t*)"Out of bounds");
        }
    } else {
        elapsed_ms = osKernelGetTickCount() - gameStartTick;

        if (elapsed_ms < defaultConfig.maze.timeToComplete) {
            remaining_ms = defaultConfig.maze.timeToComplete - elapsed_ms;
        } else {
            remaining_ms = 0;
        }

        seconds = remaining_ms / 1000;
        tenths = (remaining_ms % 1000) / 100;

        snprintf(buf, sizeof(buf), "T:%lu.%lu", seconds, tenths);
        LCD_DisplayString(70, 10, (uint8_t*)buf);
    }
}

void Display_Init(void) {
    LCD_SetFont(&Font16x24);
    LCD_Clear(0, COLOR_BG);
}
void DisplayTask(void *argument) {
    (void)argument;

    LCD_Clear(0, COLOR_BG);
    draw_maze(&g_maze);
    draw_waypoint_circle(currentWaypoint, COLOR_WP);
    draw_status(0, 0, 0);
    draw_tilt_ring();
    draw_tilt_dot(0.0f, 0.0f, COLOR_DRONE);

    float prevX = PosX;
    float prevY = PosY;
    float prevAngleX = 0.0f;
    float prevAngleY = 0.0f;
    uint8_t prevWaypoint = currentWaypoint;
    uint8_t prevGameOver = 0;
    uint32_t lastWakeTime = osKernelGetTickCount();

    while (1) {
        uint8_t localOver   = gameOver;
        uint8_t localWon    = gameWon;
        uint8_t localReason = gameOverReason;

        uint16_t r_px = (uint16_t)(defaultConfig.drone.diameter / 2);

        // DRONE ERASE
        int16_t ex = (int16_t)mm2px_x(prevX);
        int16_t ey = (int16_t)mm2px_y(prevY);
        for (int16_t dy = -(int16_t)r_px; dy <= (int16_t)r_px; dy++) {
            for (int16_t dx = -(int16_t)r_px; dx <= (int16_t)r_px; dx++) {
                if (dx*dx + dy*dy <= (int16_t)(r_px * r_px)) {
                    safe_pixel((uint16_t)(ex + dx), (uint16_t)(ey + dy), COLOR_BG);
                }
            }
        }

        // WAYPPOINT UPDATE
        uint8_t curWaypoint = currentWaypoint;
        if (curWaypoint != prevWaypoint) {
            draw_waypoint_circle(prevWaypoint, COLOR_BG);
            if (curWaypoint < defaultConfig.maze.waypoints.number) {
                draw_waypoint_circle(curWaypoint, COLOR_WP);
            }
            prevWaypoint = curWaypoint;
        }

        osMutexAcquire(g_pos_mutex, osWaitForever);
        float curX = PosX;
        float curY = PosY;
        osMutexRelease(g_pos_mutex);

        draw_drone(curX, curY);

        // STATUS UPDATE
        if (!localOver || !prevGameOver) {
            uint16_t clearRows = (localOver || localWon) ? 66 : 36;
            for (uint16_t row = 0; row < clearRows; row++) {
                draw_hline(0, row, LCD_PIXEL_WIDTH, COLOR_BG);
            }
            draw_status(localOver, localWon, localReason);
            if (localOver) {
                prevGameOver = 1;
            }
        }

        // TILT UPDATE
        osMutexAcquire(g_angle_mutex, osWaitForever);
        float curAngleX = physicalAngleX;
        float curAngleY = physicalAngleY;
        osMutexRelease(g_angle_mutex);

        draw_tilt_dot(prevAngleX, prevAngleY, COLOR_BG);
        draw_tilt_ring();
        draw_tilt_dot(curAngleX, curAngleY, COLOR_DRONE);

        prevX = curX;
        prevY = curY;
        prevAngleX = curAngleX;
        prevAngleY = curAngleY;

        lastWakeTime += 25;
        osDelayUntil(lastWakeTime);
    }
}
