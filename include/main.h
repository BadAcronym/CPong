#pragma once
//
// CPONG Core Types + Win32 helpers
// - Safer constants and color helpers
// - Clang pragma shims
// - Better Win32 include hygiene
// - Optional XInput rumble helpers
// - 7‑segment scoreboard helpers
// - Time utilities (QPC)
// - Offscreen buffer pitch + row accessor
//
// This header is C/C++ friendly.
//

// ---------- Win32 include hygiene ----------
#if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#  define NOMINMAX
#endif

#include <Windows.h>
#include <Xinput.h>   // link with Xinput9_1_0.lib or Xinput1_4.lib
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _MSC_VER
#  ifndef CPONG_NO_XINPUT_LINK
#    pragma comment(lib, "Xinput9_1_0.lib")
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------- Compiler pragmas (clang shims) ----------
#if defined(__clang__)
#  define clang_ignore_unused              _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")
#  define clang_ignore_functype_mismatch   _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wcast-function-type-mismatch\"")
#  define clang_diagnostic_pop             _Pragma("clang diagnostic pop")
#else
#  define clang_ignore_unused
#  define clang_ignore_functype_mismatch
#  define clang_diagnostic_pop
#endif

// ---------- Storage linkage helpers ----------
#define persistent  static
#define global      static
#define internal    static

// ---------- Utils ----------
#ifndef CPONG_COUNT_OF
#  define CPONG_COUNT_OF(a) (sizeof(a)/sizeof((a)[0]))
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined(__cplusplus)
#  ifndef CPONG_STATIC_ASSERT
#    define CPONG_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#  endif
#else
#  ifndef CPONG_STATIC_ASSERT
#    define CPONG_STATIC_ASSERT(cond, msg) typedef char static_assertion_##msg[(cond)?1:-1]
#  endif
#endif

// ---------- Pixel format ----------
#define CPONG_BPP 4u /* bytes per pixel: 32-bit ARGB */
CPONG_STATIC_ASSERT(CPONG_BPP == 4u, BPP_must_be_4);

// ---------- ARGB colors (0xAARRGGBB) ----------
typedef uint32_t cpong_color_t;

#define CPONG_COLOR_ARGB(a,r,g,b)   ((cpong_color_t)((((uint32_t)(a))<<24) | (((uint32_t)(r))<<16) | (((uint32_t)(g))<<8) | ((uint32_t)(b))))
#define CPONG_COLOR_RGB(r,g,b)      CPONG_COLOR_ARGB(0xFFu,(r),(g),(b))

#define CPONG_WHITE    ((cpong_color_t)0xFFFFFFFFu)
#define CPONG_BLACK    ((cpong_color_t)0xFF000000u)
#define CPONG_RED      ((cpong_color_t)0xFFFF0000u)
#define CPONG_GREEN    ((cpong_color_t)0xFF00FF00u)
#define CPONG_BLUE     ((cpong_color_t)0xFF0000FFu)
#define CPONG_MAGENTA  ((cpong_color_t)0xFFFF00FFu)

// Channel extraction (A,R,G,B in 0..255)
#define CPONG_A(c) ((uint8_t)(((c) >> 24) & 0xFF))
#define CPONG_R(c) ((uint8_t)(((c) >> 16) & 0xFF))
#define CPONG_G(c) ((uint8_t)(((c) >>  8) & 0xFF))
#define CPONG_B(c) ((uint8_t)(((c) >>  0) & 0xFF))

// ---------- Input + gameplay constants ----------
#define CPONG_DEADZONE      8192
#define CPONG_RUMBLE_USEC   6400000ULL /* 6.4 seconds default rumble window */

// Player keyboard controls (change if you need)
#define PLAYER1_UP          'W'
#define PLAYER1_DOWN        'S'
#define PLAYER2_UP          VK_UP
#define PLAYER2_DOWN        VK_DOWN

// Directions & collisions
#define CPONG_DOWN          1
#define CPONG_UP            -1

#define CPONG_NOHIT         0
#define CPONG_LEFTWALL      7
#define CPONG_RIGHTWALL     14

// ---------- Seven-segment glyphs (abcdefg DP) ----------
#define CPONG_SEGMENT_0  0x7E /* 0111 1110 */
#define CPONG_SEGMENT_1  0x30 /* 0011 0000 */
#define CPONG_SEGMENT_2  0x6D /* 0110 1101 */
#define CPONG_SEGMENT_3  0x79 /* 0111 1001 */
#define CPONG_SEGMENT_4  0x33 /* 0011 0011 */
#define CPONG_SEGMENT_5  0x5B /* 0101 1011 */
#define CPONG_SEGMENT_6  0x5F /* 0101 1111 */
#define CPONG_SEGMENT_7  0x70 /* 0111 0000 */
#define CPONG_SEGMENT_8  0x7F /* 0111 1111 */
#define CPONG_SEGMENT_9  0x7B /* 0111 1011 */

// ---------- Math helpers ----------
internal inline float cpong_clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

internal inline float cpong_signf(float x) { return (x > 0.f) - (x < 0.f); }

// ---------- Seven-segment helpers ----------
internal inline uint8_t cpong_seg_digit(uint8_t d) {
    static const uint8_t glyphs[10] = {
        CPONG_SEGMENT_0, CPONG_SEGMENT_1, CPONG_SEGMENT_2, CPONG_SEGMENT_3, CPONG_SEGMENT_4,
        CPONG_SEGMENT_5, CPONG_SEGMENT_6, CPONG_SEGMENT_7, CPONG_SEGMENT_8, CPONG_SEGMENT_9
    };
    return glyphs[d % 10u];
}

/* Pack up to 2 digits into 16 bits: [tens|ones] */
internal inline uint16_t cpong_seg_encode_u8(uint8_t value) {
    uint8_t ones = value % 10u;
    uint8_t tens = (value / 10u) % 10u;
    return (uint16_t)(((uint16_t)cpong_seg_digit(tens) << 8) | cpong_seg_digit(ones));
}

/* Pack up to 4 digits into 32/64 bits: [thousands|hundreds|tens|ones] */
internal inline uint32_t cpong_seg_encode_u16(uint16_t value) {
    uint8_t o =  value         % 10u;
    uint8_t t = (value / 10u)  % 10u;
    uint8_t h = (value / 100u) % 10u;
    uint8_t k = (value / 1000u)% 10u;
    return ((uint32_t)cpong_seg_digit(k) << 24) |
           ((uint32_t)cpong_seg_digit(h) << 16) |
           ((uint32_t)cpong_seg_digit(t) <<  8) |
           ((uint32_t)cpong_seg_digit(o) <<  0);
}

// ---------- XInput helpers ----------
internal inline float cpong_thumb_to_unit(short v, int deadzone) {
    int val = v;
    if (val > +deadzone || val < -deadzone) {
        float f = (float)val / 32767.0f;
        if (f > 1.f) f = 1.f;
        if (f < -1.f) f = -1.f;
        return f;
    }
    return 0.0f;
}

internal inline bool cpong_key_down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

// Start rumble and record when to stop (absolute QPC microseconds)
internal inline void cpong_rumble_start(DWORD user_index, WORD left, WORD right, uint64_t until_qpc_usec, uint64_t* out_until) {
    XINPUT_VIBRATION vib = { .wLeftMotorSpeed = left, .wRightMotorSpeed = right };
    XInputSetState(user_index, &vib);
    if (out_until) *out_until = until_qpc_usec;
}

internal inline void cpong_rumble_stop(DWORD user_index) {
    XINPUT_VIBRATION vib = { 0, 0 };
    XInputSetState(user_index, &vib);
}

// ---------- Time (QPC) ----------
typedef struct CpongTime {
    uint64_t qpc;   /* QueryPerformanceCounter ticks */
    uint64_t freq;  /* QueryPerformanceFrequency ticks per second */
} CpongTime;

internal inline CpongTime cpong_time_now(void) {
    LARGE_INTEGER c, f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    CpongTime t = { (uint64_t)c.QuadPart, (uint64_t)f.QuadPart };
    return t;
}

internal inline uint64_t cpong_qpc_usec(CpongTime t) {
    return (t.qpc * 1000000ULL) / (t.freq ? t.freq : 1ULL);
}

internal inline double cpong_seconds_between(CpongTime a, CpongTime b) {
    uint64_t dt = (b.qpc >= a.qpc) ? (b.qpc - a.qpc) : 0ULL;
    return (double)dt / (double)(b.freq ? b.freq : a.freq);
}

internal inline uint64_t cpong_usec_between(CpongTime a, CpongTime b) {
    uint64_t dt = (b.qpc >= a.qpc) ? (b.qpc - a.qpc) : 0ULL;
    uint64_t f  = (b.freq ? b.freq : a.freq);
    return (dt * 1000000ULL) / (f ? f : 1ULL);
}

// ---------- Basic types ----------
typedef struct Coordinate {
    float x;
    float y;
} Coordinate; /* AKA 2D vector */

typedef struct Ball {
    Coordinate coord;
    float      h_vel;
    float      v_vel;
    uint64_t   updatetime; /* QPC microseconds or game time units */
    uint32_t   size;       /* pixels */
} Ball;

typedef struct Paddles {
    Coordinate player1; /* top-left origin */
    Coordinate player2;
    uint32_t   width;
    uint32_t   height;
    float      v_vel;
    uint64_t   updatetime;
    uint64_t   player1_rumbletime;  /* absolute QPC microseconds to stop */
    uint64_t   player2_rumbletime;  /* absolute QPC microseconds to stop */
    uint64_t   lastmovetime;
    int8_t     lastmovedirection;   /* CPONG_UP / CPONG_DOWN */
    uint32_t   player1_score;
    uint32_t   player2_score;
    uint64_t   player1_sevenSegment; /* user-defined packing (e.g., cpong_seg_encode_u16) */
    uint64_t   player2_sevenSegment;
} Paddles;

typedef struct CpongControlMap {
    bool player1_up;
    bool player1_down;
    bool player2_up;
    bool player2_down;
} CpongControlMap;

typedef struct Win32WindowDimensions {
    uint32_t width;
    uint32_t height;
} Win32WindowDimensions;

typedef struct Win32OffscreenBuffer {
    BITMAPINFO info;
    void*      memory;
    uint32_t   width;
    uint32_t   height;
    uint32_t   pitch;            /* bytes per row */
    uint16_t   bytes_per_pixel;  /* should be CPONG_BPP */
} Win32OffscreenBuffer;

internal inline void cpong_buffer_init(Win32OffscreenBuffer* b, uint32_t w, uint32_t h) {
    if (!b) return;
    ZeroMemory(&b->info, sizeof(b->info));
    b->info.bmiHeader.biSize        = sizeof(b->info.bmiHeader);
    b->info.bmiHeader.biWidth       = (LONG)w;
    b->info.bmiHeader.biHeight      = -(LONG)h; /* top-down DIB */
    b->info.bmiHeader.biPlanes      = 1;
    b->info.bmiHeader.biBitCount    = (WORD)(CPONG_BPP * 8);
    b->info.bmiHeader.biCompression = BI_RGB;

    b->width           = w;
    b->height          = h;
    b->bytes_per_pixel = (uint16_t)CPONG_BPP;
    b->pitch           = w * (uint32_t)CPONG_BPP;
}

internal inline void* cpong_buffer_row(Win32OffscreenBuffer* b, uint32_t y) {
    return (uint8_t*)b->memory + y * b->pitch;
}

#ifdef __cplusplus
} // extern "C"
#endif
