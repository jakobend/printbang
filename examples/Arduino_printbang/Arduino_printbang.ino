#include <avr/pgmspace.h>
#include <avr/io.h>

#define PRINTBANG_PORT PORTB
#define PRINTBANG_PIN PB0
#define PRINTBANG_WANT_INT
#define PRINTBANG_IMPLEMENTATION
#include "printbang.h"

int i = 0;

void setup()
{
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
}

void loop()
{
    bangln(PSTR("Hello from program space!"));
    bang(PSTR("Running for ")); bang(i, 10); bangln(PSTR(" seconds."));
    delay(1000);
    i++;
}
