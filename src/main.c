#undef UNICODE

#include <Windows.h>
#include <errhandlingapi.h>
#include <stdio.h>

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
            //TODO: handle
            printf("WM_SIZE\n");
            break;
        }
        case WM_DESTROY:
        {
            printf("WM_DESTROY\n");
            PostQuitMessage(0);
            break;
        }
        case WM_CLOSE:
        {
            //TODO: handle
            printf("WM_CLOSE\n");
            break;
        }
        case WM_ACTIVATEAPP:
        {
            //TODO: handle
            printf("WM_ACTIVATEAPP\n");
            break;
        }
        case WM_PAINT:
        {
            printf("WM_PAINT\n");

            PAINTSTRUCT paintStruct;
            BeginPaint(window, &paintStruct);

            //some blitting
            // PatBlt();

            EndPaint(window, &paintStruct);
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

#ifdef DEBUG
int main()
{
    STARTUPINFO startupInfo;
    GetStartupInfoA(&startupInfo);

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
    for(;;)
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
