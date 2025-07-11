#include <Windows.h>
#include <stdio.h>

#undef UNICODE

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

    WinMain(0, 0,
            GetCommandLine(),
            0);
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

    wc.style = WS_OVERLAPPEDWINDOW;
    wc.lpfnWndProc = mainWindowCallback;
    wc.hInstance = instance;
    wc.lpszClassName = "CPongClass";

    if(!RegisterClass(&wc))
    {
        printf("unable to register window class.");
        return 99;
    };

    int x      = CW_USEDEFAULT;
    int y      = CW_USEDEFAULT;
    int width  = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;

    HWND windowHandle = CreateWindowEx(0,
                                       wc.lpszClassName,
                                       "CPong",
                                       wc.style,
                                       x, y, width, height,
                                       0, 0, instance, 0);
    if(!windowHandle)
    {
        printf("unable to obtain window handle.");
        return 87;
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
