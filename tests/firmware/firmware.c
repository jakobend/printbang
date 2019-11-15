#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "avr_mcu_section.h"
AVR_MCU(F_CPU, MCU);

#define PRINTBANG_PORT PORTB
#define PRINTBANG_PIN PB0
#define PRINTBANG_DATA_BITS 7
#define PRINTBANG_IMPLEMENTATION
#include <printbang.h>  

int main(void)
{
    DDRB |= _BV(DDB0);
    PORTB |= _BV(PB0);
    bangln(PSTR("Hello, World!"));
    
    // Stops SimAVR
    cli();
    sleep_mode();
    return 0;
}
