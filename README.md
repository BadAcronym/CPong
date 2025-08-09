# CPong
Pong, in C! A simple game, simple to play, simple to code.

This is mainly a practice project because I want to work on bigger projects (like a 3D graphics engine)
and I think it's only appropriate to have built a simple 2D game with no engine before that.

## Controls:
Player 1 (left), use either:
- `W` and `S` on the keyboard,
- `DPAD_UP` and `DPAD_DOWN` or
- the left analog stick on the controller

Player 2 (right), use either:
- `ARROW_UP` and `ARROW_DOWN` on the keyboard,
- `GAMEPAD_Y` and `GAMEPAD_A` or
- the right analog stick on the controller

![preview](preview.png)

## what it taught me:
- programming a minimal application with only the C runtime library and the WIN32 API
- writing a simple software renderer that writes to a bitmap in RAM and blits that to the window
- normalizing coordinates for positions independent of buffer or screen size
- writing simple collision checks
- utilizing delta time and velocities to keep gameplay speed consistent, no matter the hardware
- getting and processing player input (both polling & interrupt-based, for controller & keyboard)
- unifying input so controller and keyboard can be used at the same time
- finding and loading a library via `GetProcAddress()` instead of linking to a `.dll` file after compilation
- using multi-line C macros and the `_Pragma()` replacement for `#pragma`

## future plans:
- fix permanent loading icon
- simulating a 7-segment display for the current score
- sound... possibly
- abstracting platform layer and porting to X11
