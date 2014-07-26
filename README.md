HalfByte_TinyBasic
==================

A high level language for Arduino and compatibles
Half-Byte Tiny Basic is a high level language, based on the Tiny Basic from the 1970's. It is designed to be used on the Arduino or compatible directly, with out being tethered to a computer. It requires TVOut and the PS/2 keyboard library. It allows for programs upto 1000 bytes on an ATMega 328 or similar controller.  You have full control over the hardware and can even write short and simple games that utilize graphics, sound and the Wii Nunchuck.  

You must have the following libraries installed:
#include <i2cmaster.h>
#include <nunchuck.h>
#include <font4x6g.h>
#include <TVout.h>
#include <PS2Keyboard.h>
#include <EEPROM.h>

You must connect the keyboard like this:
 DataPin = 2;
 IRQpin =  3;

Video Resolution is 80 by 44. 0,0 is the upper left. 79,43 is the lower right.
That gives you about 18 characters by 8 lines. 0,0 is the upper left corner. 18,7 is the lower right.

Graphics statements for LINE, BOX, CIRCLE, SET and RESET pixels, SHIFT screen, INVERSE (reverses video)
Graphics functions for GET pixel
Sound via TONE
Hardware access via AREAD (analog), DREAD (digital), SPRINT (serial Print), OUT (serial out), AWRITE and DWRITE and IN for serial input.
ECHO controls output to SERIAL. If ECHO 1 is used, all output goes to both the video and the serial. ECHO 0 turns off serial output (but, you can still use SPRINT and OUT.) 
Wii Nunchuck support includes ability to read both buttons, analog stick and the acccelerometer all through the PAD function.
No built in string handling, but, using PEEK and POKE and CHR, you can fake string handling. It is very simple.
The interpreter is not perfect and odd things will happen.
