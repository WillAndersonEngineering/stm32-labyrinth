#include "config.h"

struct config defaultConfig = {
    .physics = {
        .gravity = 980,
        .updateFrequency = 50,
        .pinAtCenter = PIN_DRONE,
        .angleGain =  500
    },
    .drone = {
        .disruptor = {
            .maxTime = 1000,
            .power = 10000,
            .minActivationEnergy = 6000
        },
        .energyStore = {
            .maxEnergy = 15000,
            .rechargeRate = 1000
        },
        .diameter = 10
    },
    .maze = {
        .timeToComplete = 30000,
        .cellSize = 12,
        .holeDiameter = 11,
        .hardEdged = 0,

        .size = {
            .width = 15,
            .height = 15
        },
        .obstacleProbability = {
            .wall = 100,
            .hole = 200
        },
        .waypoints = {
            .number = 4,
            .diameter = 90,
            .reuse = 0,
            .location = {
                {.x = 50, .y = 50},
                {.x = 130, .y = 50},
                {.x = 130, .y = 130},
                {.x = 50, .y = 130}
            }
        }
    }
};
