#include <Windows.h>
#include <winuser.h>

LRESULT CALLBACK mainWindowCallback
(
    HWND   window,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
){
    LRESULT result = 0;

    switch(message)
    {
        case WM_SIZE:
        {
            //TODO:
            break;
        }
        case WM_DESTROY:
        {
            //TODO:
            break;
        }
        case WM_CLOSE:
        {
            //TODO:
            break;
        }
        case WM_ACTIVATEAPP:
        {
            //TODO:
            break;
        }
        default:
        {
            result = DefWindowProcA(window, message, wParam, lParam);
            break;
        }
    }

    return result;
}

int CALLBACK WinMain
(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     cmdline,
    int       cmdShow
){
    WNDCLASS wc = {0};

    wc.style = WS_OVERLAPPEDWINDOW;
    wc.lpfnWndProc = mainWindowCallback;
    wc.hInstance = instance;
    wc.lpszClassName = "CPongClass";

    if(!RegisterClassA(&wc))
    {
        return -99;
    };

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;

    CreateWindowExA(0, wc.lpszClassName, "CPong", wc.style,
                    x, y, width, height, 0, 0, instance, 0);
}
