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
    0.0021f, -0.0021f,
    0.0025f, -0.0025f,
    0.0018f, -0.0018f,
    0.0023f, -0.0023f
};

const float ball_vRNG[] =
{
    0.0025f,
    -0.0007f,
    0.0009f,
    -0.0001f,
    -0.0005f,
    0.0004f,
    -0.0008f,
    0.003f,
    0.00075f,
    -0.003f,
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
    int32_t newX_pix  = newX * width;

    bool left  = newX_pix - ball->size/2 < paddles->width;
    bool right = newX_pix + ball->size/2 > width - paddles->width;

    if(!left && !right)
    {
        return false;
    }

    int32_t ballY_pix   = newY * height;
    int32_t ball_top    = ballY_pix - ball->size/2;
    int32_t ball_bottom = ballY_pix + ball->size/2;

    int32_t paddle1Y_pix   = paddles->player1.y * height;
    int32_t paddle1_top    = paddle1Y_pix - paddles->height/2;
    int32_t paddle1_bottom = paddle1Y_pix + paddles->height/2;

    int32_t paddle2Y_pix   = paddles->player2.y * height;
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
        //TODO: if paddle is moving, change v_vel as well
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
    }
    else if(newY <= 0.0f || newX >= 1.0f)
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

//TODO: update paddles based on player input
internal void updatePlayers
(
    Paddles *paddles
){
    return;
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
    //TODO: here??
    //updatePlayers(paddles);

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
        case WM_KEYDOWN:
        {
            //TODO:
        }
        case WM_KEYUP:
        {
            //TODO:
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
    ball.h_vel = ball_hRNG[win32QueryTime().time % 8];
    ball.v_vel = ball_vRNG[win32QueryTime().time % 10];
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
