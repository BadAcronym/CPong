#undef UNICODE

#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include "main.h"

//TODO: remove globals eventually
global bool                 running;
global Win32OffscreenBuffer backbuf;

#define CPONG_WHITE 0b11111111111111111111111111111111
#define CPONG_BLACK 0b11111111000000000000000000000000
#define CPONG_RED   0b11111111111111110000000000000000
#define CPONG_GREEN 0b11111111000000001111111100000000
#define CPONG_BLUE  0b11111111000000000000000011111111

internal void updateScore
(
    Score *score,
    Ball  *ball
){
    if(ball->h_vel > 0.0f)
    {
        score->player += 1;
    }
    else
    {
        score->enemy += 1;
    }

    printf("player: %d, enemy: %d\n", score->player, score->enemy);

    ball->h_vel *= 1.025f;
}

internal void updateBall
(
    Ball       *ball,
    Score      *score
){
    Time t2 = win32QueryTime();

    uint64_t delta_int = t2.time - ball->stamp;
    delta_int *= 1000000;

    float delta_float = (float)delta_int / t2.freq;
    delta_float /= 4096;

    ball->stamp = t2.time;

    float newX = ball->coord.x + delta_float * ball->h_vel;
    float newY = ball->coord.y + delta_float * ball->v_vel;

    //TODO: handle player collision

    if(newX > 1.0f || newX < 0.0f)
    {
        updateScore(score, ball);
        ball->h_vel *= -1;
        return;
    }

    if(newY > 1.0f || newY < 0.0f)
    {
        ball->v_vel *= -1;
        return;
    }

    ball->coord.x = newX;
    ball->coord.y = newY;
}

internal void updateBackbuffer
(
    Win32OffscreenBuffer *buf,
    Ball                 *ball,
    Score                *score
){
    updateBall(ball, score);

    uint32_t *pixel = (uint32_t*)buf->memory;

    int bar_width  = buf->width / 256;
    int bar_height = buf->height / 32;

    int ball_size  = buf->width / 128;
    float ballX = ball->coord.x * buf->width;
    float ballY = ball->coord.y * buf->height;

    for(size_t i = buf->height; i > 0; --i)
    {
        for(size_t j = 0; j < buf->width; ++j)
        {

            if(j < ballX + ball_size &&
               j > ballX - ball_size &&
               i < ballY + ball_size &&
               i > ballY - ball_size
            ){
                *pixel++ = CPONG_WHITE;
            }
            else if((i / bar_height) % 2 == 1      &&
                    j > (buf->width/2 - bar_width) &&
                    j < (buf->width/2 + bar_width)
            ){
                *pixel++ = CPONG_WHITE;
            }
            else if(0) //TODO: case for drawing players
            {
            }
            else if(0) //TODO: player score (left)
            {
            }
            else if(0) //TODO: enemy score (right)
            {
            }
            else
            {
                *pixel++ = CPONG_BLACK;
            }
        }
    }
}

const float ballRNG[] =
{
        0.001f,
        -0.0007f,
        0.0009f,
        -0.001f,
        0.0008f,
        -0.0008f,
        0.00075f
};

internal float getRandomBallVelocity()
{
    return ballRNG[win32QueryTime().time % 7];
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

            Win32WindowDimensions dim = win32GetWindowDimensions(window);

            win32BltBuf(backbuf, context, dim.width, dim.height);

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

    win32ResizeDIBSection(&backbuf, 1280, 720);

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

    Ball ball = {0};
    ball.coord.x = 0.5f;
    ball.coord.y = 0.5f;
    ball.h_vel = getRandomBallVelocity();
    ball.v_vel = getRandomBallVelocity();
    ball.stamp = win32QueryTime().time;

    Score score = {0};

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

        updateBackbuffer(&backbuf, &ball, &score);

        HDC context = GetDC(window);

        Win32WindowDimensions dim = win32GetWindowDimensions(window);

        win32BltBuf(backbuf, context, dim.width, dim.height);

        ReleaseDC(window, context);
    }

    return 0;
}
