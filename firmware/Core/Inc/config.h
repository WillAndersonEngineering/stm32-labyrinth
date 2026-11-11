#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_
#include <stdint.h>
#include <stdio.h>

typedef enum
{
    PIN_DRONE = 0,
    PIN_MAZE = 1
} pin_at_center_t;

struct physics
{
    uint16_t gravity;
    uint8_t updateFrequency;
    pin_at_center_t pinAtCenter;
    uint16_t angleGain;
};

//DRONE

struct disruptor
{
    uint16_t maxTime;
    uint16_t power;
    uint16_t minActivationEnergy;
};

struct energy_store
{
    uint16_t maxEnergy;
    uint16_t rechargeRate;
};

struct drone
{
    struct disruptor disruptor;
    struct energy_store energyStore;
    uint16_t diameter;
};

//MAZE
struct size
{
    uint8_t width;
    uint8_t height;
};

struct obstacleProbability
{
    uint16_t wall;
    uint16_t hole;
};

struct waypoint_location
{
    uint16_t x;
    uint16_t y;
};

struct waypoints
{
    uint8_t number;
    uint8_t diameter;
    uint8_t reuse; //BOOL
    struct waypoint_location location[4];
};

struct maze
{
    uint16_t timeToComplete;
    uint16_t cellSize;
    uint16_t holeDiameter;
    uint8_t hardEdged; //BOOL

    struct size size;
    struct obstacleProbability obstacleProbability;
    struct waypoints waypoints;
};

//CONFIG
struct config
{
    uint8_t version;
    struct physics physics;
    struct drone drone;
    struct maze maze;
};

extern struct config defaultConfig;
    
#endif /* INC_CONFIG_H_ */