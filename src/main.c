#undef UNICODE

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"

//TODO: remove globals eventually
global bool       running;

global BITMAPINFO bitmapInfo;
global void       *bitmapMemory;
global uint32_t   bitmapWidth;
global uint32_t   bitmapHeight;
global uint32_t   bpp = 4;

internal void renderBits()
{
    //test
    uint8_t *pixel_channel = (uint8_t*)bitmapMemory;
    uint8_t brightness = ((bitmapWidth/128) * (bitmapHeight/128));

    uint32_t bitmapMemorySize = bitmapWidth * bitmapHeight * bpp;

    for(size_t i = 0; i < bitmapMemorySize; i += bpp)
    {
        *pixel_channel = brightness;
        ++pixel_channel;

        *pixel_channel = brightness;
        ++pixel_channel;

        *pixel_channel = brightness;
        ++pixel_channel;

        //padding
        ++pixel_channel;
    }
}

internal void win32ResizeDIBSection
(
    uint32_t width,
    uint32_t height
){
    if(bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    uint32_t bitmapMemorySize = bitmapWidth * bitmapHeight * bpp;

    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void win32UpdateWindow
(
    HDC       deviceContext,
    RECT      *windowRect
){
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;

    StretchDIBits(deviceContext,
                  0, 0, bitmapWidth, bitmapHeight,
                  0, 0, windowWidth, windowHeight,
                  bitmapMemory, &bitmapInfo,
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

            win32ResizeDIBSection(width, height);
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

            win32UpdateWindow(context, &clientRect);

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

    MSG message;
    while(running)
    {
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

        renderBits();

        HDC context = GetDC(window);

        RECT clientRect;
        GetClientRect(window, &clientRect);
        win32UpdateWindow(context, &clientRect);

        ReleaseDC(window, context);
    }

    return 0;
}
