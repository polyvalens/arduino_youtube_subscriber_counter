This is the WIZnet library Ethernet-W6100.zip with the file utility/w5100.cpp modified at line 36
to set the SS pin to pin 3:

`33   ...`
`34   #elif defined(__AVR__)`
`35   //#define SS_PIN_DEFAULT  10`
`36   #define SS_PIN_DEFAULT  3`
`37   ...`

Use it in place of the default Arduino Ethernet library.

WIZnet IoT iOffload Contest
Clemens Valens
August 2019
