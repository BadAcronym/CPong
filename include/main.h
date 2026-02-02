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
    uint64_t   stamp;
    uint32_t   size;
}Ball;

typedef struct
{
    Coordinate player1;
    Coordinate player2;
    uint32_t   leftscore;
    uint32_t   rightscore;
    uint32_t   width;
    uint32_t   height;
}Paddles;
