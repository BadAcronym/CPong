#pragma once

#define persistent  static
#define global      static
#define internal    static

#include <stdint.h>

typedef struct
{
    float x;
    float y;
}Coordinate;

typedef struct
{
    Coordinate coord;
    float      h_vel;
    float      v_vel;
}Ball;

typedef struct
{
    uint32_t player;
    uint32_t enemy;
}Score;
