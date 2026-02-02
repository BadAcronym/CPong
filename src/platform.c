#include <Windows.h>
#include <stdbool.h>

#include "platform.h"

Win32WindowDimensions win32GetWindowDimensions
(
    HWND window
){
    RECT clientRect;
    GetClientRect(window, &clientRect);

    Win32WindowDimensions result;
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;

    return result;
}

void win32ResizeDIBSection
(
    Win32OffscreenBuffer *buf,
    uint32_t             width,
    uint32_t             height
){
    if(buf->memory)
    {
        VirtualFree(buf->memory, 0, MEM_RELEASE);
    }

    buf->width = width;
    buf->height = height;
    buf->bpp = 4;

    buf->info.bmiHeader.biSize = sizeof(buf->info.bmiHeader);
    buf->info.bmiHeader.biWidth = buf->width;
    buf->info.bmiHeader.biHeight = buf->height;
    buf->info.bmiHeader.biPlanes = 1;
    buf->info.bmiHeader.biBitCount = 32;
    buf->info.bmiHeader.biCompression = BI_RGB;

    uint32_t bitmapMemorySize = buf->width * buf->height * buf->bpp;

    buf->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void win32BltBuf
(
    Win32OffscreenBuffer buf,
    HDC                  deviceContext,
    uint32_t             width,
    uint32_t             height
){
    StretchDIBits(deviceContext,
                  0, 0, width, height,
                  0, 0, buf.width, buf.height,
                  buf.memory, &buf.info,
                  DIB_RGB_COLORS, SRCCOPY);
}

long long win32GetTimestamp()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart;
}
