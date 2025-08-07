#undef UNICODE

#include "main.h"

#include <stdio.h>

//XInput Shenanigans... thanks Casey :)
clang_ignore_unused

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetState_Stub)
{
    return 0;
}
global x_input_get_state *XInputGetState_ = XInputGetState_Stub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetState_Stub)
{
    return 0;
}
global x_input_set_state *XInputSetState_ = XInputSetState_Stub;
#define XInputSetState XInputSetState_

clang_diagnostic_pop

//TODO: remove globals eventually
global bool                 global_running;
global Win32OffscreenBuffer global_backbuffer;

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

    buf->info.bmiHeader.biSize = sizeof(buf->info.bmiHeader);
    buf->info.bmiHeader.biWidth = buf->width;
    buf->info.bmiHeader.biHeight = buf->height;
    buf->info.bmiHeader.biPlanes = 1;
    buf->info.bmiHeader.biBitCount = 32;
    buf->info.bmiHeader.biCompression = BI_RGB;

    uint32_t bitmapMemorySize = buf->width * buf->height * CPONG_BPP;

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

Time win32QueryTime(void)
{
    LARGE_INTEGER timestamp;
    LARGE_INTEGER frequency;
    Time t1;

    QueryPerformanceCounter(&timestamp);
    QueryPerformanceFrequency(&frequency);
    t1.time = timestamp.QuadPart;
    t1.freq = frequency.QuadPart;

    return t1;
}

void win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        clang_ignore_functype_mismatch

        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");

        clang_diagnostic_pop
    }
}

const float ball_hRNG[] =
{
    0.00077f, -0.00077f,
    0.00080f, -0.00080f,
    0.00085f, -0.00085f,
    0.00090f, -0.00090f,
};

const float ball_vRNG[] =
{
    0.0025f, -0.0007f,
    0.0009f, -0.0001f,
    -0.0005f, 0.0004f,
    -0.0008f, 0.003f,
    0.00075f, -0.003f
};

internal bool checkPaddleCollision
(
    uint32_t width,
    uint32_t height,
    Paddles  *paddles,
    Ball     *ball,
    float    newX,
    float    newY
){
    int32_t newX_pix = (int32_t)(newX * width);

    bool left  = newX < 0.0f || newX_pix - ball->size/2 < paddles->width;
    bool right = newX > 1.0f || newX_pix + ball->size/2 > width - paddles->width;

    if(!left && !right)
    {
        return false;
    }

    int32_t ballY_pix   = (int32_t)(newY * height);
    int32_t ball_top    = ballY_pix - ball->size/2;
    int32_t ball_bottom = ballY_pix + ball->size/2;

    int32_t paddle1Y_pix   = (int32_t)(paddles->player1.y * height);
    int32_t paddle1_top    = paddle1Y_pix - paddles->height/2;
    int32_t paddle1_bottom = paddle1Y_pix + paddles->height/2;

    int32_t paddle2Y_pix   = (int32_t)(paddles->player2.y * height);
    int32_t paddle2_top    = paddle2Y_pix - paddles->height/2;
    int32_t paddle2_bottom = paddle2Y_pix + paddles->height/2;

    if(left && ball->h_vel < 0.0f)
    {
        return (ball_top < paddle1_bottom && ball_top > paddle1_top) ||
               (ball_bottom < paddle1_bottom && ball_bottom > paddle1_top);
    }

    return ball->h_vel > 0.0f &&
           ((ball_top < paddle2_bottom && ball_top > paddle2_top) ||
            (ball_bottom < paddle2_bottom && ball_bottom > paddle2_top));
}

internal void bounceBallCheck
(
    uint32_t width,
    uint32_t height,
    Paddles  *paddles,
    Ball     *ball,
    float    newX,
    float    newY
){
    if(checkPaddleCollision(width, height, paddles, ball, newX, newY))
    {
        //TODO: if paddle is moving, change v_vel as well and increase it by a tiny flat amount
        //maybe if it hits the center of the paddle and it's not moving, decrease magnitude of v_vel?
        ball->h_vel *= -1.1f;
    }
    else if(newX <= 0.0f || newX >= 1.0f)
    {
        if(newX >= 1.0f)
        {
            ++paddles->leftscore;
        }
        if(newX <= 0.0f)
        {
            ++paddles->rightscore;
        }

        printf("left: %d, right: %d\n", paddles->leftscore, paddles->rightscore);
        float scoreMod = 1 + 3 * (paddles->leftscore + paddles->rightscore) / 10000.0f;
        ball->h_vel = ball_hRNG[win32QueryTime().time % 8] * scoreMod;
        ball->v_vel = ball_vRNG[win32QueryTime().time % 10];

        ball->coord.x = 0.5f;
        ball->coord.y = 0.5f;

        paddles->v_vel *= scoreMod;
    }
    else if(newY <= 0.0f || newY >= 1.0f)
    {
        ball->v_vel *= -1.0f;
    }
    else
    {
        ball->coord.x = newX;
        ball->coord.y = newY;
    }
}

internal float getDeltaTime
(
    uint64_t t1
){
    Time t2 = win32QueryTime();

    uint64_t delta_int = t2.time - t1;
    delta_int *= 1000000;

    float delta_float = (float)delta_int / t2.freq;
    return delta_float / 4096;
}

internal void updateBall
(
    uint32_t width,
    uint32_t height,
    Paddles *paddles,
    Ball    *ball
){
    float delta = getDeltaTime(ball->stamp);
    ball->stamp = win32QueryTime().time;

    float newX = ball->coord.x + delta * ball->h_vel;
    float newY = ball->coord.y + delta * ball->v_vel;

    bounceBallCheck(width, height, paddles, ball, newX, newY);
}

internal void updatePaddles
(
    uint32_t        height,
    CpongControlMap *controlMap,
    Paddles         *paddles
){
    float delta = getDeltaTime(paddles->stamp);
    paddles->stamp = win32QueryTime().time;

    if(!controlMap->playerIndex)
    {
        if(controlMap->up)
        {
            float newY_player1_up = paddles->player1.y - delta * paddles->v_vel;
            if(newY_player1_up * height - paddles->height/2 > 0)
            {
                paddles->player1.y = newY_player1_up;
            }
        }
        if(controlMap->down)
        {
            float newY_player1_down = paddles->player1.y + delta * paddles->v_vel;
            if(newY_player1_down * height + paddles->height/2 < height)
            {
                paddles->player1.y = newY_player1_down;
            }
        }
    }
    else if(controlMap->playerIndex)
    {
        if(controlMap->up)
        {
            float newY_player2_up = paddles->player2.y - delta * paddles->v_vel;
            if(newY_player2_up * height - paddles->height/2 > 0)
            {
                paddles->player2.y = newY_player2_up;
            }
        }
        if(controlMap->down)
        {
            float newY_player2_down = paddles->player2.y + delta * paddles->v_vel;
            if(newY_player2_down * height + paddles->height/2 < height)
            {
                paddles->player2.y = newY_player2_down;
            }
        }
    }
}

internal void updateBackbuffer
(
    Win32OffscreenBuffer *buf,
    Paddles              *paddles,
    Ball                 *ball
){
    uint32_t *pixel = (uint32_t*)buf->memory;

    int bar_width  = buf->width / 256;
    int bar_height = buf->height / 32;

    paddles->width  = buf->width / 128;
    paddles->height = buf->height / 8;

    ball->size = buf->width / 128;

    float ballX = ball->coord.x * buf->width;
    float ballY = ball->coord.y * buf->height;

    updateBall(buf->width, buf->height, paddles, ball);

    for(size_t i = buf->height; i > 0; --i)
    {
        for(size_t j = 0; j < buf->width; ++j)
        {

            if(j < ballX + ball->size &&                            //ball
               j > ballX - ball->size &&
               i < ballY + ball->size &&
               i > ballY - ball->size
            ){
                *pixel++ = CPONG_WHITE;
            }
            else if((i / bar_height) % 2 == 1      &&               //middle bar
                    j > (buf->width/2 - bar_width) &&
                    j < (buf->width/2 + bar_width)
            ){
                *pixel++ = CPONG_WHITE;
            }                                                       //paddles
            else if((j < paddles->width                                       &&
                     i < paddles->player1.y * buf->height + paddles->height/2 &&
                     i > paddles->player1.y * buf->height - paddles->height/2)
                    ||
                    (j > buf->width - paddles->width                          &&
                     i < paddles->player2.y * buf->height + paddles->height/2 &&
                     i > paddles->player2.y * buf->height - paddles->height/2)
            ){
                *pixel++ = CPONG_WHITE;
            }
            else if(0) //TODO: player 1 score (left)
            {
            }
            else if(0) //TODO: player 2 score (right)
            {
            }
            else
            {
                *pixel++ = CPONG_BLACK;
            }
        }
    }
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
        case WM_DESTROY:
        {
            //TODO: handle this as error, recreate window?
            printf("WM_DESTROY\n");
            break;
        }
        case WM_CLOSE:
        {
            //TODO: handle this with message box or prompt
            global_running = false;
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

            Win32WindowDimensions dim = win32GetWindowDimensions(window);

            win32BltBuf(global_backbuffer, context, dim.width, dim.height);

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

clang_ignore_unused
int CALLBACK WinMain
(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR     cmdline,
    int       cmdShow
){

    win32LoadXInput();

    global_running = true;

    win32ResizeDIBSection(&global_backbuffer, 1280, 720);

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

    Paddles paddles = {0};
    paddles.v_vel = 0.003f;
    paddles.player1.x = 0.0f;
    paddles.player1.y = 0.5f;
    paddles.player2.x = 1.0f;
    paddles.player2.y = 0.5f;
    paddles.stamp = win32QueryTime().time;

    Ball ball = {0};
    ball.coord.x = 0.5f;
    ball.coord.y = 0.5f;
    ball.h_vel = ball_hRNG[win32QueryTime().time % 8];
    ball.v_vel = ball_vRNG[win32QueryTime().time % 10];
    ball.stamp = win32QueryTime().time;

    if(!window)
    {
        printf("unable to obtain window handle.");
        return GetLastError();
    }

    while(global_running)
    {
        MSG message;

        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
        {
            if(message.message == WM_QUIT)
            {
                global_running = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        for(DWORD controlIndex = 0; controlIndex < XUSER_MAX_COUNT; ++controlIndex)
        {
            XINPUT_STATE controlState;
            if(XInputGetState(controlIndex, &controlState) == ERROR_SUCCESS)
            {
                XINPUT_GAMEPAD *pad = &controlState.Gamepad;

                CpongControlMap controlMap;
                controlMap.playerIndex = (bool)controlIndex;
                controlMap.up   = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                controlMap.down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;

                updatePaddles(global_backbuffer.height, &controlMap, &paddles);
            }
        }

        updateBackbuffer(&global_backbuffer, &paddles, &ball);

        HDC context = GetDC(window);

        Win32WindowDimensions dim = win32GetWindowDimensions(window);

        win32BltBuf(global_backbuffer, context, dim.width, dim.height);

        ReleaseDC(window, context);
    }

    return 0;
}
clang_diagnostic_pop
