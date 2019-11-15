#include <util/delay.h>
#include <avr/pgmspace.h>

#define PRINTBANG_PORT PORTB
#define PRINTBANG_PIN PB0
#define PRINTBANG_IMPLEMENTATION
#include "printbang.h"

int main(void)
{
    PRINTBANG_PORT |= _BV(PRINTBANG_PIN);
    DDRB |= _BV(PRINTBANG_PIN);

    for (int i = 0;; i++)
    {
        bangln(PSTR("Hello from program space!"));
        bang(PSTR("Running for ")); bang(i, 10); bangln(" seconds.");
        _delay_ms(1000);
    }
    return 0;
}
