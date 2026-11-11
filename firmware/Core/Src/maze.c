// maze.c

#include "maze.h"

static uint32_t Maze_Rand(void) {
#if USE_HAL_RNG
    uint32_t v = 0;
    HAL_RNG_GenerateRandomNumber(&hrng, &v);
    return v;
#else
    return (uint32_t)rand();
#endif
}

static uint8_t Maze_RollProb(uint16_t prob_per_1000) {
    return (Maze_Rand() % 1000) < prob_per_1000;
}

static float PointToSeg(float pointx, float pointy, float ax, float ay, float bx, float by) {
    float vx = bx - ax;
    float vy = by - ay;
    float wx = pointx - ax;
    float wy = pointy - ay;
    float c1 = (vx*wx) + (vy*wy);

    if (c1 <= 0) {
        return wx*wx + wy*wy;
    }

    float c2 = (vx*vx) + (vy*vy);
    if (c2 <= c1) {
        float dx = pointx - bx;
        float dy = pointy - by;
        return (dx*dx) + (dy*dy);
    }

    float t = c1 / c2;
    float ex = (ax + t*vx) - pointx;
    float ey = (ay + t*vy) - pointy;
    return (ex*ex) + (ey*ey);
}

void Maze_Generate(maze_t *maze, const struct config *cfg){
    uint8_t W = cfg->maze.size.width;
    uint8_t H = cfg->maze.size.height;

    if( W > MAZE_MAX_WIDTH)
        W = MAZE_MAX_WIDTH;
    if( H > MAZE_MAX_HEIGHT)
        H = MAZE_MAX_HEIGHT;
    
    maze->width = W;
    maze->height = H;
    maze->cellSize = cfg->maze.cellSize;

    // CLEAR
    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 0; c < W; c++)
            maze->hWall[r][c] = 0;
    }

    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 0; c < W; c++)
            maze->vWall[r][c] = 0;
    }

    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 0; c < W; c++)
            maze->hole[r][c] = 0;
    }

    //HARD EDGE WALLS
    if (cfg->maze.hardEdged) {
        for (uint8_t c = 0; c < W; c++) {
            maze->hWall[0][c] = 1;
            maze->hWall[H][c] = 1;
        }
        
        for (uint8_t r = 0; r < H; r++) {
            maze->vWall[r][0] = 1;
            maze->vWall[r][W] = 1;
        }
    }

    // INSIDE WALLS
    uint16_t wallProb = cfg->maze.obstacleProbability.wall;

    for (uint8_t r = 1; r < H; r++) {
        for (uint8_t c = 0; c < W; c++) {
            if (Maze_RollProb(wallProb))
                maze->hWall[r][c] = 1;
        }
    }

    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 1; c < W; c++) {
            if (Maze_RollProb(wallProb))
                maze->vWall[r][c] = 1;
        }
    }

    // HOLES
    uint16_t holeProb = cfg->maze.obstacleProbability.hole;
    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 0; c < W; c++) {
            if (Maze_RollProb(holeProb))
                maze->hole[r][c] = 1;
        }
    }

    // SAFE AREA AROUND ALL WAYPOINTS
    float cellSize = (float)maze->cellSize;
    float holeKeepout = (float)cfg->maze.holeDiameter;
    float holeKeepoutSq = holeKeepout * holeKeepout;
    float wallKeepout = cellSize / 2.0f;
    float wallKeepoutSq = wallKeepout * wallKeepout;

    for (uint8_t wp = 0; wp < cfg->maze.waypoints.number; wp++) {
        float wpX = (float)cfg->maze.waypoints.location[wp].x;
        float wpY = (float)cfg->maze.waypoints.location[wp].y;

        for (uint8_t r = 0; r < H; r++) {
            for (uint8_t c = 0; c < W; c++) {
                if (maze->hole[r][c]) {
                    float holeCX = (c + 0.5f) * cellSize;
                    float holeCY = (r + 0.5f) * cellSize;
                    float dx = holeCX - wpX;
                    float dy = holeCY - wpY;
                    if ((dx*dx + dy*dy) < holeKeepoutSq) {
                        maze->hole[r][c] = 0;
                    }
                }
            }
        }

        // WALLS NEXT TO WAYPOINTS
        for (uint8_t r = 0; r <= H; r++) {
            for (uint8_t c = 0; c < W; c++) {
                if (maze->hWall[r][c]) {
                    float ax = c * cellSize,       ay = r * cellSize;
                    float bx = (c + 1) * cellSize, by = r * cellSize;
                    if (PointToSeg(wpX, wpY, ax, ay, bx, by) < wallKeepoutSq) {
                        maze->hWall[r][c] = 0;
                    }
                }
            }
        }

        for (uint8_t r = 0; r < H; r++) {
            for (uint8_t c = 1; c < W; c++) {
                if (maze->vWall[r][c]) {
                    float ax = c * cellSize, ay = r * cellSize;
                    float bx = c * cellSize, by = (r + 1) * cellSize;
                    if (PointToSeg(wpX, wpY, ax, ay, bx, by) < wallKeepoutSq) {
                        maze->vWall[r][c] = 0;
                    }
                }
            }
        }
    }

    // REMOVE HOLES NEAR SPAWN
    float spawnX = (float)(W * (uint16_t)maze->cellSize) / 2.0f;
    float spawnY = (float)(H * (uint16_t)maze->cellSize) / 2.0f;
    float droneRadius = (float)cfg->drone.diameter / 2.0f;
    float holeDetectR = (float)cfg->maze.holeDiameter / 2.0f;
    float spawnKeepout = droneRadius + holeDetectR + cellSize;
    float spawnKeepoutSq = spawnKeepout * spawnKeepout;

    for (uint8_t r = 0; r < H; r++) {
        for (uint8_t c = 0; c < W; c++) {
            if (maze->hole[r][c]) {
                float holeCX = (c + 0.5f) * cellSize;
                float holeCY = (r + 0.5f) * cellSize;
                float dx = holeCX - spawnX;
                float dy = holeCY - spawnY;
                if ((dx*dx + dy*dy) < spawnKeepoutSq) {
                    maze->hole[r][c] = 0;
                }
            }
        }
    }
}

uint8_t Maze_HoleAt( const maze_t *maze, float x, float y, float radius) {  
        float cellSize = (float)maze->cellSize;
        float radiusSq = radius * radius;
        for (uint8_t r = 0; r < maze->height; r++) {
            for (uint8_t c = 0; c < maze->width; c++) {
                if (maze->hole[r][c]) {
                    float holeCenterX = (c + 0.5f) * cellSize;
                    float holeCenterY = (r + 0.5f) * cellSize;
                    float dx = holeCenterX - x;
                    float dy = holeCenterY - y;

                    if ((dx*dx + dy*dy) < radiusSq)
                        return 1;           
                }
            }
        }
    return 0;
}
