#undef UNICODE

#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include "main.h"

//TODO: remove globals eventually
global bool                 running;
global Win32OffscreenBuffer backbuf;

#define CPONG_WHITE   0b11111111111111111111111111111111
#define CPONG_BLACK   0b11111111000000000000000000000000
#define CPONG_RED     0b11111111111111110000000000000000
#define CPONG_GREEN   0b11111111000000001111111100000000
#define CPONG_BLUE    0b11111111000000000000000011111111
#define CPONG_MAGENTA 0b11111111111111110000000011111111

const float ball_hRNG[] =
{
    0.001f,
    -0.0007f,
    0.0009f,
    -0.001f,
    0.0008f,
    -0.0008f,
    0.00075f
};

const float ball_vRNG[] =
{
    0.00025f,
    0.0f,
    -0.00007f,
    0.00009f,
    -0.00001f,
    -0.000005f,
    0.00004f,
    -0.0008f,
    0.0000003f,
    0.00075f,
    -0.0003f,
    0.0f
};

//FIXME: player collision checks
internal bool checkPaddleCollision
(
    uint32_t width,
    uint32_t height,
    Paddles  *paddles,
    Ball     *ball
){
    uint32_t ballX_pix = ball->coord.x * width;

    bool left = ballX_pix - ball->size/2 < paddles->width;
    bool right = ballX_pix + ball->size/2 > width - paddles->width;

    if(!left && !right)
    {
        return false;
    }

    uint32_t ballY_pix = ball->coord.y * height;
    uint32_t ball_top  = ballY_pix - ball->size/2;
    uint32_t ball_bottom  = ballY_pix + ball->size/2;

    uint32_t paddle1Y_pix    = paddles->player1.y * height;
    uint32_t paddle1_top    = paddle1Y_pix - paddles->height/2;
    uint32_t paddle1_bottom = paddle1Y_pix + paddles->height/2;

    uint32_t paddle2Y_pix = paddles->player2.y * height;
    uint32_t paddle2_top    = paddle2Y_pix - paddles->height/2;
    uint32_t paddle2_bottom = paddle2Y_pix + paddles->height/2;

    if(left)
    {
        return (ball_top < paddle1_bottom    &&
                ball_top > paddle1_top)
               ||
               (ball_bottom < paddle1_bottom &&
                ball_bottom > paddle1_top);
    }
    else
    {
        return (ball_top < paddle2_bottom    &&
                ball_top > paddle2_top)
               ||
               (ball_bottom < paddle2_bottom &&
                ball_bottom > paddle2_top);
    }
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
    if(checkPaddleCollision(width, height, paddles, ball))
    {
        ball->h_vel *= -1.05f;
    }
    else if(newX >= 1.0f || newX <= 0.0f)
    {
        //to reduce code paths, instead of re-checking if(newX >= 1.0f){...}
        paddles->leftscore += (int)newX;
        paddles->rightscore += (1 + (int)newX) % 2;

        printf("left: %d, right: %d\n", paddles->leftscore, paddles->rightscore);
        float scoreMod = 1 + 3 * (paddles->leftscore + paddles->rightscore) / 10000.0f;
        ball->h_vel = ball_hRNG[win32QueryTime().time % 7] * scoreMod;

        ball->coord.x = 0.5f;
        ball->coord.y = 0.5f;
    }
    else if(newY >= 1.0f || newY <= 0.0f)
    {
        ball->v_vel *= -1.0f;
    }
    else
    {
        ball->coord.x = newX;
        ball->coord.y = newY;
    }
}

internal void updateBall
(
    uint32_t width,
    uint32_t height,
    Paddles *paddles,
    Ball    *ball
){
    Time t2 = win32QueryTime();

    uint64_t delta_int = t2.time - ball->stamp;
    delta_int *= 1000000;

    float delta_float = (float)delta_int / t2.freq;
    delta_float /= 4096;

    ball->stamp = t2.time;

    float newX = ball->coord.x + delta_float * ball->h_vel;
    float newY = ball->coord.y + delta_float * ball->v_vel;

    bounceBallCheck(width, height, paddles, ball, newX, newY);
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
            else if((j < paddles->width                                     &&
                     i < paddles->player1.y * buf->height + paddles->height &&
                     i > paddles->player1.y * buf->height - paddles->height)
                    ||
                    (j > buf->width - paddles->width                        &&
                     i < paddles->player2.y * buf->height + paddles->height &&
                     i > paddles->player2.y * buf->height - paddles->height)
            ){
                *pixel++ = CPONG_WHITE;
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

    Paddles paddles = {0};
    paddles.player1.x = 0.0f;
    paddles.player1.y = 0.5f;
    paddles.player2.x = 1.0f;
    paddles.player2.y = 0.5f;

    Ball ball = {0};
    ball.coord.x = 0.5f;
    ball.coord.y = 0.5f;
    ball.h_vel = ball_hRNG[win32QueryTime().time % 7];
    ball.v_vel = ball_vRNG[win32QueryTime().time % 12];
    ball.stamp = win32QueryTime().time;

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

        updateBackbuffer(&backbuf, &paddles, &ball);

        HDC context = GetDC(window);

        Win32WindowDimensions dim = win32GetWindowDimensions(window);

        win32BltBuf(backbuf, context, dim.width, dim.height);

        ReleaseDC(window, context);
    }

    return 0;
}
