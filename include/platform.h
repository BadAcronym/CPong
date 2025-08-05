#pragma once

#include <stdint.h>
#include <windows.h>

#define CPONG_BPP     4

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

extern LRESULT CALLBACK win32WindowCallback
(
    HWND   window,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
);

extern Win32WindowDimensions win32GetWindowDimensions
(
    HWND window
);

extern void win32ResizeDIBSection
(
    Win32OffscreenBuffer *buf,
    uint32_t             width,
    uint32_t             height
);

extern void win32BltBuf
(
    Win32OffscreenBuffer buf,
    HDC                  deviceContext,
    uint32_t             width,
    uint32_t             height
);

extern Time win32QueryTime();
