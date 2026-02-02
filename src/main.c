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
            printf("WM_SIZE");
            break;
        }
        case WM_DESTROY:
        {
            //TODO: handle
            printf("WM_DESTROY");
            break;
        }
        case WM_CLOSE:
        {
            //TODO: handle
            printf("WM_CLOSE");
            break;
        }
        case WM_ACTIVATEAPP:
        {
            //TODO: handle
            printf("WM_ACTIVATEAPP");
            break;
        }
        case WM_PAINT:
        {
            //TODO: handle
            printf("WM_PAINT");
        }
        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
            break;
        }
    }

    return result;
}

#ifdef DEBUG
int main()
{
    STARTUPINFO startupInfo;
    GetStartupInfo(&startupInfo);

    return WinMain(GetModuleHandle(NULL), 0, GetCommandLine(), 0);
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

    if(!RegisterClass(&wc))
    {
        printf("unable to register window class.");
        return GetLastError();
    };

    int x      = CW_USEDEFAULT;
    int y      = CW_USEDEFAULT;
    int width  = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;

    HWND windowHandle = CreateWindowEx(0,
                                       wc.lpszClassName,
                                       "CPong",
                                       WS_OVERLAPPEDWINDOW,
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
        BOOL msgResult = GetMessage(&message, 0, 0, 0);

        if(msgResult <= 0)
        {
            break;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
