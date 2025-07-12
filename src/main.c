#undef UNICODE

#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>

#include "main.h"

global bool running = true;

LRESULT CALLBACK mainWindowCallback
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
            //TODO: handle correctly
            printf("WM_SIZE\n");
            break;
        }
        case WM_DESTROY:
        {
            //TODO: handle this as error, recreate window?
            printf("WM_DESTROY\n");
            running = false;
            break;
        }
        case WM_CLOSE:
        {
            //TODO: handle this with message box or prompt
            printf("WM_CLOSE\n");
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
            BeginPaint(window, &paintStruct);

            int x = paintStruct.rcPaint.left;
            int y = paintStruct.rcPaint.top;
            int width = paintStruct.rcPaint.right - paintStruct.rcPaint.left;
            int height = paintStruct.rcPaint.bottom - paintStruct.rcPaint.top;

            PatBlt(paintStruct.hdc, x, y, width, height, BLACKNESS);

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
    return WinMain(GetModuleHandleA(NULL), 0, GetCommandLineA(), 0);
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

    wc.lpfnWndProc = mainWindowCallback;
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

    HWND windowHandle = CreateWindowExA(0, wc.lpszClassName, "CPong",
                                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        x, y, width, height,
                                        0, 0, instance, 0);
    if(!windowHandle)
    {
        printf("unable to obtain window handle.");
        return GetLastError();
    }

    MSG message;
    while(running)
    {
        BOOL msgResult = GetMessageA(&message, 0, 0, 0);

        if(msgResult <= 0)
        {
            break;
        }

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return 0;
}
