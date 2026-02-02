#pragma once

#include <Windows.h>
#include <Xinput.h>

#include <stdbool.h>
#include <stdint.h>

#define CPONG_BPP     4

#define persistent  static
#define global      static
#define internal    static

#define clang_ignore_unused\
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wunused-parameter\"") \

#define clang_ignore_functype_mismatch\
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wcast-function-type-mismatch\"") \

#define clang_diagnostic_pop\
    _Pragma("clang diagnostic pop")\

#define CPONG_WHITE   0b11111111111111111111111111111111
#define CPONG_BLACK   0b11111111000000000000000000000000
#define CPONG_RED     0b11111111111111110000000000000000
#define CPONG_GREEN   0b11111111000000001111111100000000
#define CPONG_BLUE    0b11111111000000000000000011111111
#define CPONG_MAGENTA 0b11111111111111110000000011111111

#define CPONG_DEADZONE 8000

typedef struct Coordinate
{
    float x;
    float y;
}Coordinate;

typedef struct Ball
{
    Coordinate coord;
    float      h_vel;
    float      v_vel;
    uint64_t   stamp;
    uint32_t   size;
}Ball;

typedef struct Paddles
{
    Coordinate player1;
    Coordinate player2;
    uint32_t   leftscore;
    uint32_t   rightscore;
    uint32_t   width;
    uint32_t   height;
    float      v_vel;
    uint64_t   player1stamp;
    uint64_t   player2stamp;
}Paddles;

typedef struct CpongControlMap
{
    uint8_t playerIndex;
    bool    up;
    bool    down;
}CpongControlMap;

typedef struct Win32WindowDimensions
{
    uint32_t width;
    uint32_t height;
}
Win32WindowDimensions;

typedef struct Win32OffscreenBuffer
{
    BITMAPINFO info;
    void       *memory;
    uint32_t   width;
    uint32_t   height;
}
Win32OffscreenBuffer;

typedef struct
{
    uint64_t time;
    uint64_t freq;
}Time;
