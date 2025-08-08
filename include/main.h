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

#define CPONG_DEADZONE   8192
#define CPONG_RUMBLETIME 6400000

#define PLAYER1_UP       'W'
#define PLAYER1_DOWN     'S'
#define PLAYER2_UP       VK_UP
#define PLAYER2_DOWN     VK_DOWN

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
    uint64_t   updatetime;
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
    uint64_t   updatetime;
    uint64_t   player1_rumbletime;
    uint64_t   player2_rumbletime;
}Paddles;

typedef struct CpongControlMap
{
    bool    player1_up;
    bool    player1_down;
    bool    player2_up;
    bool    player2_down;
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
