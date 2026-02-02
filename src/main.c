#undef UNICODE

#include "main.h"
#include <assert.h>

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

//remove globals eventually... or never?
global bool                 global_running;
global Win32OffscreenBuffer global_backbuffer;
global CpongControlMap      global_controllerMap;
global CpongControlMap      global_keyMap;
global Paddles              global_paddles;

Win32WindowDimensions win32GetWindowDimensions
(
    HWND window
){
    RECT clientRect;
    GetClientRect(window, &clientRect);

    Win32WindowDimensions result;
    result.width  = clientRect.right  - clientRect.left;
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
    Win32OffscreenBuffer *buf,
    HDC                  deviceContext,
    uint32_t             width,
    uint32_t             height
){
    StretchDIBits(deviceContext,
                  0, 0, width, height,
                  0, 0, buf->width, buf->height,
                  buf->memory, &buf->info,
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

internal uint8_t checkHorizontalCollision
(
    uint32_t width,
    Ball     *ball,
    float    newX
){
    int32_t newX_pix = (int32_t)(newX * width);

    if ((newX <= 0.0f) ||
        (newX_pix - ball->size/2 <= 0)
    ){
        return CPONG_LEFTWALL;
    }
    else if((newX >= 1.0f) ||
            (newX_pix + ball->size/2 >= width)
    ){
        return CPONG_RIGHTWALL;
    }

    return CPONG_NOHIT;
}

internal bool checkVerticalCollision
(
    uint32_t height,
    Ball     *ball,
    float    newY
){
    int32_t newY_pix = (int32_t)(newY * height);

    return (newY <= 0.0f || newY >= 1.0f) ||
           (newY_pix - ball->size/2 <= 0) ||
           (newY_pix + ball->size/2 >= height);
}

internal void resetRumble
(
    Paddles *paddles
){
    XINPUT_VIBRATION rumble;
    rumble.wLeftMotorSpeed  = 0;
    rumble.wRightMotorSpeed = 0;

    Time t2 = win32QueryTime();

    uint64_t delta1 = t2.time - paddles->player1_rumbletime;
    if(delta1 > CPONG_RUMBLETIME)
    {
        XInputSetState(0, &rumble);
    }

    uint64_t delta2 = t2.time - paddles->player2_rumbletime;
    if(delta2 > CPONG_RUMBLETIME)
    {
        XInputSetState(1, &rumble);
    }
}

internal void rumblePlayer
(
    Paddles *paddles,
    uint8_t playerIndex
){
    XINPUT_VIBRATION rumble;
    rumble.wLeftMotorSpeed  = 40000;
    rumble.wRightMotorSpeed = 40000;
    XInputSetState(playerIndex, &rumble);

    if(playerIndex == 0)
    {
        paddles->player1_rumbletime = win32QueryTime().time;
    }
    else if(playerIndex == 1)
    {
        paddles->player2_rumbletime = win32QueryTime().time;
    }
}

internal bool isSevenSegment
(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    Paddles  *paddles
){
    uint32_t segment_height = height / 32;
    if(y > segment_height * 2)
    {
        return false;
    }

    int8_t pos_mod = 1;
    if(x < width / 2)
    {
        pos_mod = -1;
    }

    uint16_t segment_gap   = (uint16_t)(width / 128);
    uint16_t segment_width = (uint16_t)(width / 32);

    for(uint32_t i = 0; i < 64; i+=8)
    {
        int64_t distance = pos_mod * i * segment_width + segment_gap;
    }

    //TODO:

    return false;
}

internal const uint64_t segments[] =
{
    CPONG_SEGMENT_0, CPONG_SEGMENT_1,
    CPONG_SEGMENT_2, CPONG_SEGMENT_3,
    CPONG_SEGMENT_4, CPONG_SEGMENT_5,
    CPONG_SEGMENT_6, CPONG_SEGMENT_7,
    CPONG_SEGMENT_8, CPONG_SEGMENT_9
};

//little endian, meaning the last 8 bits of the return value
//represent the 0th digit, the second to last 8 bits the 1st
//
//technically can go up to 8 digit scores, but who the fuck...
//1 digit = 1 byte, each leading 0 and then segments on/off
internal uint64_t translateToSevenSegment
(
    uint32_t score
){
    uint64_t digitMask = 0;
    uint32_t mod = 10;

    for(size_t i = 0; i < 64; i+=8)
    {
        uint32_t digit = (score * 10 / mod) % mod;
        assert(digit < 10);

        digitMask = digitMask | (segments[digit] << i);

        if(score < mod)
        {
            return digitMask;
        }
        mod *= 10;
    }

    for(size_t i = 0; i < 64; i+=8)
    {
        digitMask = CPONG_SEGMENT_9 & (CPONG_SEGMENT_9 << i);
    }

    return digitMask;
}

internal void incrementPlayerScore
(
    Paddles *paddles,
    uint8_t playerIndex
){
    if(playerIndex == 0)
    {
        paddles->player1_score += 1;
        paddles->player1_sevenSegment = translateToSevenSegment(paddles->player1_score);
    }
    else if(playerIndex == 1)
    {
        paddles->player2_score += 1;
        paddles->player2_sevenSegment = translateToSevenSegment(paddles->player2_score);
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

internal void checkBallBounce
(
    uint32_t width,
    uint32_t height,
    Paddles  *paddles,
    Ball     *ball,
    float    newX,
    float    newY
){
    uint8_t horizontal_result = checkHorizontalCollision(width, ball, newX);

    if(checkPaddleCollision(width, height, paddles, ball, newX, newY))
    {
        ball->h_vel *= -1.1f;

        float delta = getDeltaTime(paddles->lastmovetime);
        if(delta < 0.001f)
        {
            ball->v_vel += (paddles->lastmovedirection * paddles->v_vel / 4);
        }
    }
    else if(horizontal_result)
    {
        if(horizontal_result == CPONG_LEFTWALL)
        {
            incrementPlayerScore(paddles, 1);
            rumblePlayer(paddles, 0);
        }
        else if(horizontal_result == CPONG_RIGHTWALL)
        {
            incrementPlayerScore(paddles, 0);
            rumblePlayer(paddles, 1);
        }

        //debug
        printf("player1: %d, player2: %d\n", paddles->player1_score, paddles->player2_score);

        float scoreMod = 1 + 3 * (paddles->player1_score + paddles->player2_score) / 10000.0f;
        ball->h_vel = ball_hRNG[win32QueryTime().time % 8] * scoreMod;
        ball->v_vel = ball_vRNG[win32QueryTime().time % 10];

        ball->coord.x = 0.5f;
        ball->coord.y = 0.5f;

        paddles->v_vel *= scoreMod;
    }
    else if(checkVerticalCollision(height, ball, newY))
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
    Paddles  *paddles,
    Ball     *ball
){
    float delta = getDeltaTime(ball->updatetime);
    ball->updatetime = win32QueryTime().time;

    float newX = ball->coord.x + delta * ball->h_vel;
    float newY = ball->coord.y + delta * ball->v_vel;

    checkBallBounce(width, height, paddles, ball, newX, newY);
}

internal void updatePaddles
(
    uint32_t        height,
    Paddles         *paddles
){
    float delta = getDeltaTime(paddles->updatetime);
    paddles->updatetime = win32QueryTime().time;

    float newY_player1_up   = paddles->player1.y - delta * paddles->v_vel;
    float newY_player1_down = paddles->player1.y + delta * paddles->v_vel;

    bool player1_up   = global_controllerMap.player1_up   || global_keyMap.player1_up;
    bool player1_down = global_controllerMap.player1_down || global_keyMap.player1_down;

    bool player2_up   = global_controllerMap.player2_up   || global_keyMap.player2_up;
    bool player2_down = global_controllerMap.player2_down || global_keyMap.player2_down;

    if(player1_up && newY_player1_up * height - paddles->height/2 > 0)
    {
        paddles->player1.y         = newY_player1_up;
        paddles->lastmovetime      = paddles->updatetime;
        paddles->lastmovedirection = CPONG_UP;
    }
    if(player1_down && newY_player1_down * height + paddles->height/2 < height)
    {
        paddles->player1.y         = newY_player1_down;
        paddles->lastmovetime      = paddles->updatetime;
        paddles->lastmovedirection = CPONG_DOWN;
    }

    float newY_player2_up   = paddles->player2.y - delta * paddles->v_vel;
    float newY_player2_down = paddles->player2.y + delta * paddles->v_vel;

    if(player2_up && newY_player2_up * height - paddles->height/2 > 0)
    {
        paddles->player2.y         = newY_player2_up;
        paddles->lastmovetime      = paddles->updatetime;
        paddles->lastmovedirection = CPONG_UP;
    }
    if(player2_down && newY_player2_down * height + paddles->height/2 < height)
    {
        paddles->player2.y         = newY_player2_down;
        paddles->lastmovetime      = paddles->updatetime;
        paddles->lastmovedirection = CPONG_DOWN;
    }
}

//TODO: move middle bar into a struct? maybe.
//some kind of animation when scoring, on the score and the paddle who scored perhaps
internal void updateBackbuffer
(
    Win32OffscreenBuffer *buf,
    Paddles              *paddles,
    Ball                 *ball
){
    uint32_t *pixel = (uint32_t*)buf->memory;

    int bar_width  = buf->width  / 256;
    int bar_height = buf->height / 32;

    paddles->width  = buf->width  / 128;
    paddles->height = buf->height / 8;

    ball->size = buf->width / 128;

    float ballX = ball->coord.x * buf->width;
    float ballY = ball->coord.y * buf->height;

    for(uint32_t i = buf->height; i > 0; --i)
    {
        for(uint32_t j = 0; j < buf->width; ++j)
        {

            if(j < ballX + ball->size &&
               j > ballX - ball->size &&
               i < ballY + ball->size &&
               i > ballY - ball->size
            ){
                *pixel++ = CPONG_WHITE;
            }
            else if((i / bar_height) % 2 == 1      &&
                    j > (buf->width/2 - bar_width) &&
                    j < (buf->width/2 + bar_width)
            ){
                *pixel++ = CPONG_WHITE;
            }
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
            else if(isSevenSegment(j, i, buf->width, buf->height, paddles))
            {
                *pixel++ = CPONG_WHITE;
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

            win32BltBuf(&global_backbuffer, context, dim.width, dim.height);

            EndPaint(window, &paintStruct);
            break;
        }
        //NOTE: idk why we need to intercept WM_KEYDOWN? I'm guessing the DefWindowProc
        //mangles our signals for the WM_KEYUP case we're handling below...
        case WM_KEYDOWN:
        {
        }
        case WM_KEYUP:
        {
            bool wasKeyDown = (lParam & (1 << 30)) != 0;
            bool isKeyDown  = (lParam & (1 << 31)) == 0;

            if(wasKeyDown == isKeyDown)
            {
                break;
            }

            if(wParam == PLAYER1_UP)
            {
                global_keyMap.player1_up = isKeyDown;
            }
            else if(wParam == PLAYER1_DOWN)
            {
                global_keyMap.player1_down = isKeyDown;
            }
            else if(wParam == PLAYER2_UP)
            {
                global_keyMap.player2_up = isKeyDown;
            }
            else if(wParam == PLAYER2_DOWN)
            {
                global_keyMap.player2_down = isKeyDown;
            }
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

    global_paddles.player1.x  = 0.0f;
    global_paddles.player1.y  = 0.5f;
    global_paddles.player2.x  = 1.0f;
    global_paddles.player2.y  = 0.5f;
    global_paddles.v_vel      = 0.003f;
    global_paddles.updatetime = win32QueryTime().time;

    Ball ball = {0};
    ball.coord.x    = 0.5f;
    ball.coord.y    = 0.5f;
    ball.h_vel      = ball_hRNG[win32QueryTime().time % 8];
    ball.v_vel      = ball_vRNG[win32QueryTime().time % 10];
    ball.updatetime = win32QueryTime().time;

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

        resetRumble(&global_paddles);

        for(DWORD controlIndex = 0; controlIndex < XUSER_MAX_COUNT; ++controlIndex)
        {
            XINPUT_STATE controlState;
            if(XInputGetState(controlIndex, &controlState) == ERROR_SUCCESS)
            {
                XINPUT_GAMEPAD *pad = &controlState.Gamepad;
                global_controllerMap.player1_up   = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP ||
                                                    pad->sThumbLY > CPONG_DEADZONE;

                global_controllerMap.player1_down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN ||
                                                    pad->sThumbLY < -CPONG_DEADZONE;

                global_controllerMap.player2_up   = pad->wButtons & XINPUT_GAMEPAD_Y ||
                                                    pad->sThumbRY > CPONG_DEADZONE;

                global_controllerMap.player2_down = pad->wButtons & XINPUT_GAMEPAD_A ||
                                                    pad->sThumbRY < -CPONG_DEADZONE;

            }
        }

        updatePaddles(global_backbuffer.height, &global_paddles);
        updateBall(global_backbuffer.width, global_backbuffer.height, &global_paddles, &ball);

        updateBackbuffer(&global_backbuffer, &global_paddles, &ball);

        HDC context = GetDC(window);

        Win32WindowDimensions dim = win32GetWindowDimensions(window);

        win32BltBuf(&global_backbuffer, context, dim.width, dim.height);

        ReleaseDC(window, context);
    }

    return 0;
}
clang_diagnostic_pop
