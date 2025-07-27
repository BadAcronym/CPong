#undef UNICODE

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"

typedef struct Win32OffscreenBuffer
{
    BITMAPINFO info;
    void       *memory;
    uint32_t   width;
    uint32_t   height;
    uint32_t   bpp;
}
Win32OffscreenBuffer;

//TODO: remove globals eventually
global bool                 running;
global Win32OffscreenBuffer backbuf;

internal void renderBits(Win32OffscreenBuffer *buf)
{
    //test
    uint32_t *pixel = (uint32_t*)buf->memory;
    uint32_t brightness = ((buf->width/121) * (buf->height/121));

    uint32_t bitmapMemorySize = buf->width * buf->height * buf->bpp;

    for(size_t i = 0; i < bitmapMemorySize; i += buf->bpp)
    {
        *pixel++ = (brightness << 16) | (brightness << 8) | brightness;
    }
}

internal void win32ResizeDIBSection
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

internal void win32BltBuf
(
    Win32OffscreenBuffer buf,
    HDC                  deviceContext,
    RECT                 windowRect
){
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    StretchDIBits(deviceContext,
                  0, 0, buf.width, buf.height,
                  0, 0, windowWidth, windowHeight,
                  buf.memory, &buf.info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK win32WindowCallback
(
    HWND   window,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
){
    switch(message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(window, &clientRect);

            uint32_t width = clientRect.right - clientRect.left;
            uint32_t height = clientRect.bottom - clientRect.top;

            #ifdef DEBUG
                printf("%dx", width);
                printf("%d\n", height);
            #endif

            win32ResizeDIBSection(&backbuf, width, height);
            break;
        }
        case WM_DESTROY:
        {
            //TODO: handle this as error, recreate window?
            printf("WM_DESTROY\n");
            break;
        }
        case WM_CLOSE:
        {
            //TODO: handle this with message box or prompt
            running = false;
            break;
        }
        case WM_ACTIVATEAPP:
        {
            //TODO: handle at all
            printf("WM_ACTIVATEAPP\n");
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC context = BeginPaint(window, &paintStruct);

            RECT clientRect;
            GetClientRect(window, &clientRect);

            win32BltBuf(backbuf, context, clientRect);

            EndPaint(window, &paintStruct);
            break;
        }
        default:
        {
            return DefWindowProcA(window, message, wParam, lParam);
        }
    }

    return 0;
}

#ifdef DEBUG
int main()
{
    return WinMain(GetModuleHandleA(0), 0, GetCommandLineA(), 0);
}
#endif

int CALLBACK WinMain
(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     cmdline,
    int       cmdShow
){
    //-Wunused-parameter
    (void)prevInstance;
    (void)cmdline;
    (void)cmdShow;

    running = true;

    WNDCLASS wc = {0};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = win32WindowCallback;
    wc.hInstance = instance;
    wc.lpszClassName = "CPongClass";

    if(!RegisterClassA(&wc))
    {
        printf("unable to register window class.");
        return GetLastError();
    };

    int x      = CW_USEDEFAULT;
    int y      = CW_USEDEFAULT;
    int width  = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;

    HWND window = CreateWindowExA(0, wc.lpszClassName, "CPong",
                                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        x, y, width, height,
                                        0, 0, instance, 0);
    if(!window)
    {
        printf("unable to obtain window handle.");
        return GetLastError();
    }

    while(running)
    {
        MSG message;

        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if(message.message == WM_QUIT)
            {
                running = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        renderBits(&backbuf);

        HDC context = GetDC(window);

        RECT clientRect;
        GetClientRect(window, &clientRect);
        win32BltBuf(backbuf, context, clientRect);

        ReleaseDC(window, context);
    }

    return 0;
}
