# CPong
Pong, in C! A simple game, simple to play, simple to code.

This is mainly a practice project because I want to work on bigger projects (like a 3D graphics engine)
and I think it's only appropriate to have built a simple 2D game with no engine before that.

what it taught me:
- programming a minimal application with only the C runtime library and the WIN32 API
- writing a simple software renderer that writes to a bitmap in RAM and blits that to the window
- normalizing coordinates for positions independent of buffer or screen size
- writing simple collision checks
- utilizing delta time and velocities to keep gameplay speed consistent, no matter the hardware
- getting and processing player input (both polling & interrupt-based, for controller & keyboard)
- unifying input so controller and keyboard can be used at the same time

plans:
- simulating a 7-segment display for the current score
- gameplay improvements, like simulating friction between paddle and ball

I won't worry about porting this to linux for now. It is, after all, meant for practice.
I usually code in fairly modern, but C-Style C++ and C99 felt like a good point to go back to and be forced
to simplify some problems. And it's paid off.

<!-- ## Player Controls: -->
