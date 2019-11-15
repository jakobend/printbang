/*
MIT License

Copyright (c) 2019 Jakob Endrikat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Double-star comment blocks and triple-slash single-line comments will be
// extracted into README.md by "make readme"

/**
# [printbang](https://github.com/jakobend/printbang)

A modest serial transmitter for AVR MCUs in restricted circumstances.

```c
#define PRINTBANG_PORT PORTB
#define PRINTBANG_PIN PB0
#define PRINTBANG_WANT_INT
#define PRINTBANG_IMPLEMENTATION
#include "printbang.h"
// ...
for (int i = 0;; i++)
{
    bangln(PSTR("Hello from program space!"));
    bang(PSTR("Running for ")); bang(i, 10); bangln(PSTR(" seconds."));
    _delay_ms(1000);
}
```

## Why?

For AVR projects where you just don't have any more leeway than a digital pin,
a few clock cycles, and a couple of flash bytes.

## How?

printbang implements a cycle-counted transmission routine that is speed-limited
by an user-defined assembly snippet. Instead of relying on a buffer like other
software serial implementations, the function is called once for every word.
This also minimizes the cycles spent in time-critical code.

## Features

- No dependency on timers or hardware UARTs/USI
- Supports 1-8 data bits, LSB- or MSB-first transmission and even/odd parity
- 250000 baud default configuration for common clock frequencies
- Basic Arduino Serial-style formatting for numeric data types
- Doesn't depend on the Arduino core or the C++ runtime
- Doesn't depend on avr-libc formatting
- State- and heapless

## Caveats

- Recursive formatting routines can result in heavy stack load for long integers
- Interrupts are masked during transmission of a word
**/

#ifndef PRINTBANG_H
#define PRINTBANG_H

#include <math.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define PRINTBANG_VERSION_MAJOR 0
#define PRINTBANG_VERSION_MINOR 1

/// ## Documentation
/// ### Configuration macros

/**
#### `HAVE_PRINTBANG_CONFIG_H` ([source]({anchor}))
If this macro is defined, `<printbang_config.h>` will be included before
`printbang.h`.
**/
#ifdef HAVE_PRINTBANG_CONFIG_H
#include <printbang_config.h>
#endif

/**
#### `PRINTBANG_PORT` and `PRINTBANG_PORT_IO` ([source]({anchor}))
Either of these macros define the port of the pin used for serial output.
**/
#if !defined(PRINTBANG_PORT) && !defined(PRINTBANG_PORT_IO)
#error "printbang: either PRINTBANG_PORT or PRINTBANG_PORT_IO must be defined"
#endif

/**
If `PRINTBANG_PORT_IO` is not defined, it will be derived from `PRINTBANG_PORT`
by subtracting the SFR memory offset from it:
```c
#define PRINTBANG_PORT PORTA
// is equivalent to
#define PRINTBANG_PORT_IO _SFR_IO_ADDR(PORTA)
```
**/
#ifndef PRINTBANG_PORT_IO
#define PRINTBANG_PORT_IO _SFR_IO_ADDR(PRINTBANG_PORT)
#endif

/**
#### `PRINTBANG_PIN` and `PRINTBANG_PIN_MASK` ([source]({anchor}))
Either of these macros define the pin(s) on the chosen port to be used for
serial output.
**/
#if !defined(PRINTBANG_PIN) && !defined(PRINTBANG_PIN_MASK)
#error "printbang: either PRINTBANG_PIN or PRINTBANG_PIN_MASK must be defined"
#endif

/**
If `PRINTBANG_PIN_MASK` is not defined, it will be derived from `PRINTBANG_PIN`
by converting it into a bit mask:
```c
#define PRINTBANG_PIN PA0
// is equivalent to
#define PRINTBANG_PIN_MASK _BV(PA0)
```
**/
#ifndef PRINTBANG_PIN_MASK
#define PRINTBANG_PIN_MASK _BV(PRINTBANG_PIN)
#endif

/**
#### `PRINTBANG_DELAY` ([source]({anchor}))
This macro is an inline assembly snippet that limits the speed of the
transmission routine to a particular baudrate. If it is not defined, the
following defaults are used for common clock frequencies:
**/
#ifndef PRINTBANG_DELAY
#if F_CPU == 16500000
/// - 16.5MHz: 250000 baud, 58 delay cycles, 0% deviation
#define PRINTBANG_DELAY \
    "\n\t" "ldi r18, 19" \
    "\n" "%=:" \
    "\n\t" "dec r18" \
    "\n\t" "brne %=b" \
    "\n\t" "nop"
#define PRINTBANG_DELAY_CLOBBER "r18"

#elif F_CPU == 16000000
/// - 16MHz: 250000 baud, 56 delay cycles, 0% deviation
#define PRINTBANG_DELAY \
    "\n\t" "ldi r18, 18" \
    "\n" "%=:" \
    "\n\t" "dec r18" \
    "\n\t" "brne %=b" \
    "\n\t" "rjmp ."
#define PRINTBANG_DELAY_CLOBBER "r18"

#elif F_CPU == 8000000
/// - 8MHz: 250000 baud, 24 delay cycles, 0% deviation
#define PRINTBANG_DELAY \
    "\n\t" "ldi r18, 8" \
    "\n" "%=:" \
    "\n\t" "dec r18" \
    "\n\t" "brne %=b"
#define PRINTBANG_DELAY_CLOBBER "r18"

#elif F_CPU == 4000000
/// - 4MHz: 250000 baud, 8 delay cycles, 0% deviation
#define PRINTBANG_DELAY \
    "\n\t" "lpm" \
    "\n\t" "lpm" \
    "\n\t" "rjmp 0"

#else
#error "printbang: clock frequency not defined or not supported by automatic configuration"
#endif // F_CPU
#endif // PRINTBANG_DELAY

/**
#### `PRINTBANG_DELAY_CLOBBER` ([source]({anchor}))
This macro will be used as the clobber section of the inline assembly and allows
delay snippets to clobber registers, e.g. for looping.

TODO: Use a temporary variable instead
**/
#ifndef PRINTBANG_DELAY_CLOBBER
#define PRINTBANG_DELAY_CLOBBER
#endif

/**
#### `PRINTBANG_PARITY_EVEN` and `PRINTBANG_PARITY_ODD` ([source]({anchor}))
If one of these macros is defined, a bit with the given parity will be appended
to every transmitted word. This functionality depends on avr-libc's
`util/parity.h`.
**/
#if defined(PRINTBANG_PARITY_EVEN) && defined(PRINTBANG_PARITY_ODD)
#error "printbang: multiple parities defined"
#endif

/**
#### `PRINTBANG_DATA_BITS` ([source]({anchor}))
This macro defines the number of data bits transmitted per word. Counting always
starts at the least significant bit; if MSB-first transmission is used, the byte
will be aligned to the left side.

```c
// Saves one bit per word but limits transmission to ASCII characters
#define PRINTBANG_DATA_BITS 7
```
**/
#ifndef PRINTBANG_DATA_BITS
#define PRINTBANG_DATA_BITS 8
#endif

/**
#### `PRINTBANG_ORDER_MSB` ([source]({anchor}))
If this macro is defined, transmission will occur in MSB-first order. Otherwise,
LSB-first order will be used.
**/
#ifndef PRINTBANG_ORDER_MSB
#define PRINTBANG_ORDER_LSB
#endif

/**
#### `PRINTBANG_LINE_ENDING` ([source]({anchor}))
This macro expands to a string literal that will be used by `bangln` to
terminate a line. It defaults to `"\r\n"`.
**/
#ifndef PRINTBANG_LINE_ENDING
#define PRINTBANG_LINE_ENDING "\r\n"
#endif

/**
#### `PRINTBANG_IMPLEMENTATION` ([source]({anchor}))
printbang is a *header-only* library. When including it, its functions are
declared, but only defined if this macro is set.

The recommended way of using printbang in your project involves setting
`PRINTBANG_IMPLEMENTATION` in a single source file, most commonly wherever your
main function is located. All other source files can then include `printbang.h`
without defining this macro.
**/
#ifndef PRINTBANG_IMPLEMENTATION

void bang_char(char value);
void bang_str(const char *str);
void bang_pstr(PGM_P str);

/**
Note that the `PRINTBANG_WANT_x` macros need to be defined whenever their
respective functions should be declared.
**/
#ifdef PRINTBANG_WANT_INT
void bang_uint(unsigned int value, unsigned char base);
void bang_int(int value, unsigned char base);
#endif
#ifdef PRINTBANG_WANT_LONG
void bang_ulong(unsigned long value, unsigned char base);
void bang_long(long value, unsigned char base);
#endif
#ifdef PRINTBANG_WANT_LONG_LONG
void bang_ulonglong(unsigned long long value, unsigned char base);
void bang_longlong(long long value, unsigned char base);
#endif
#ifdef PRINTBANG_WANT_FLOAT
void bang_float(float value, unsigned char places);
#endif

#else // PRINTBANG_IMPLEMENTATION

static const PROGMEM char printbang_line_ending[] = PRINTBANG_LINE_ENDING;

#if defined(PRINTBANG_PARITY_ODD) || defined(PRINTBANG_PARITY_EVEN)
#include <util/parity.h>
#endif

/// ### Character and string transmission

/**
#### `void bang_char(char value)` ([source]({anchor}))
Transmits a single word over the serial pin. Interrupts are masked during the
runtime of this function.
**/
void bang_char(char value)
{
    cli();
    unsigned char port_value = PRINTBANG_PORT;
    unsigned char bits_remaining = PRINTBANG_DATA_BITS;

    // Align byte to the left side
#if defined(PRINTBANG_ORDER_MSB) && PRINTBANG_DATA_BITS != 8
    value <<= (8 - PRINTBANG_DATA_BITS);
#endif


#if defined(PRINTBANG_PARITY_ODD) || defined(PRINTBANG_PARITY_EVEN)
    unsigned char parity = parity_even_bit(value);
#endif

    // Every section needs to execute in 8 cycles for accurate timing combined
    // with PRINTBANG_DELAY.
    asm volatile (
        // Transmit the start bit
        "\n\t" "cbr %[port_value], %[pin_mask]"
        "\n\t" "out %[port_io], %[port_value]"
        "\n\t" "lpm"

        "\n" "1:"
        "\n\t" PRINTBANG_DELAY


        // Transmit one bit of the byte
        "\n\t" "sbr %[port_value], %[pin_mask]"
        // Shift out a bit into carry
#ifndef PRINTBANG_ORDER_MSB
        "\n\t" "ror %[value]"
#else
        "\n\t" "rol %[value]"
#endif
        "\n\t" "brcs 2f"
        "\n\t" "cbr %[port_value], %[pin_mask]"

        "\n" "2:"
        "\n\t" "out %[port_io], %[port_value]"
        "\n\t" "dec %[bits_remaining]"
        "\n\t" "brne 1b"
        "\n\t" "nop"
        "\n\t" PRINTBANG_DELAY


        // Transmit the parity bit
#if defined(PRINTBANG_PARITY_ODD) || defined(PRINTBANG_PARITY_EVEN)
        "\n\t" "sbr %[port_value], %[pin_mask]"
        "\n\t" "tst %[parity]"
#ifdef PRINTBANG_PARITY_EVEN
        "\n\t" "brne parity_high"
#else
        "\n\t" "breq parity_high"
#endif
        "\n\t" "cbr %[port_value], %[pin_mask]"
        "\n" "parity_high:"
        "\n\t" "out %[port_io], %[port_value]"
        "\n\t" "lpm"
        "\n\t" PRINTBANG_DELAY
#endif
        "\n\t" "lpm"


        // Transmit the stop bit
        "\n\t" "sbr %[port_value], %[pin_mask]"
        "\n\t" "out %[port_io], %[port_value]"
        "\n\t" PRINTBANG_DELAY

        : // Outputs
            [value] "=r" (value),
            [bits_remaining] "=r" (bits_remaining),
            [port_value] "=r" (port_value)
        : // Inputs
            "0" (value),
            "1" (bits_remaining),
            "2" (port_value),
#if defined(PRINTBANG_PARITY_ODD) || defined(PRINTBANG_PARITY_EVEN)
            [parity] "r" (parity),
#endif
            [pin_mask] "i" (PRINTBANG_PIN_MASK),
            [port_io] "i" (PRINTBANG_PORT_IO)
        : // Clobbers
            PRINTBANG_DELAY_CLOBBER
    );
    sei();
}

/**
#### `void bang_str(const char *str)` ([source]({anchor}))
Transmits a null-terminated string from RAM. Calling this function on a
program-space string will result in garbage being transmitted.
**/
void bang_str(const char *str)
{
    char chr;
    while ((chr = *str++) != '\0')
    {
        bang_char(chr);
    }
}

/**
#### `void bang_pstr(PGM_P str)` ([source]({anchor}))
Transmits a null-terminated string from program space. Calling this function on
a RAM string will result in garbage being transmitted.
**/
void bang_pstr(PGM_P str)
{
    char chr;
    while ((chr = pgm_read_byte(str++)) != '\0')
    {
        bang_char(chr);
    }
}

/// ### Integer and floating point transmission

// Generic template for integer types
#define _DEFINE_BANG_INT(T, NU, NS) \
void NU(unsigned T value, unsigned char base) \
{ \
    if (base < 2 || base > 36) return; \
    unsigned T div = value / base; \
    unsigned T mod = value % base; \
    if (div > 0) \
    { \
        NU(div, base); \
    } \
    bang_char((base > 10 && mod >= 10) ? ('A' - 10) + mod : '0' + mod); \
} \
void NS(T value, unsigned char base) \
{ \
    if (value < 0) \
    { \
        bang_char('-'); \
        NU(-value, base); \
    } \
    else \
    { \
        NU(value, base); \
    } \
}

/**
#### `PRINTBANG_WANT_INT` ([source]({anchor}))
#### `void bang_uint(unsigned int value, unsigned char base)`
#### `void bang_int(int value, unsigned char base)`
If `PRINTBANG_WANT_INT` is defined, the functions for transmitting
`unsigned int` and `int` values will be declared or defined. The passed value is
formatted in a given `base`.
**/
#ifdef PRINTBANG_WANT_INT
_DEFINE_BANG_INT(int, bang_uint, bang_int);
#endif

/**
#### `PRINTBANG_WANT_LONG` ([source]({anchor}))
#### `void bang_ulong(unsigned long value, unsigned char base)`
#### `void bang_long(long value, unsigned char base)`
If `PRINTBANG_WANT_LONG` is defined, the functions for transmitting
`unsigned long` and `long` values will be declared or defined. The passed value
is formatted in a given `base`.
**/
#ifdef PRINTBANG_WANT_LONG
_DEFINE_BANG_INT(long, bang_ulong, bang_long);
#endif

/**
#### `PRINTBANG_WANT_LONG_LONG` ([source]({anchor}))
#### `void bang_ulonglong(unsigned long long value, unsigned char base`
#### `void bang_longlong(long long value, unsigned char base)`
If `PRINTBANG_WANT_LONG_LONG` is defined, the functions for transmitting
`unsigned long long` and `long long` values will be declared or defined. The
passed value is formatted in a given `base`.
**/
#ifdef PRINTBANG_WANT_LONG_LONG
_DEFINE_BANG_INT(long long, bang_ulonglong, bang_longlong);
#endif

#undef _DEFINE_BANG_INT

/**
#### `PRINTBANG_WANT_FLOAT` ([source]({anchor}))
#### `void bang_float(float value, unsigned char base)`
If `PRINTBANG_WANT_FLOAT` is defined, the function for transmitting `float`
values will be declared or defined. The floating point formatting is very
rudimentary and will simply concatenate the number to a given number of decimal
`places`. One trailing zero is always appended.

Since `double` is an alias for `float` in avr-libc, this function should be used
for `double` values as well.
**/
#ifdef PRINTBANG_WANT_FLOAT
void bang_float(float value, unsigned char places)
{
    if (isnan(value))
    {
        bang_pstr(PSTR("nan"));
        return;
    }
    if (value < 0.0f)
    {
        value = -value;
        bang_char('-');
    }
    if (isinf(value))
    {
        bang_pstr(PSTR("inf"));
        return;
    }
    unsigned int integral_part = (unsigned int)(value);
    bang_uint(integral_part, 10);
    bang_char('.');

    value -= integral_part;
    unsigned char digit = 0;
    do
    {
        value = (value - digit) * 10;
        digit = (unsigned char)(value);
        bang_char('0' + digit);
    } while (value > 0 && (--places));
}
#endif

/**
#### `void bang(...)` ([source]({anchor}))
`bang` provides a simple generic wrapper to all `bang_x` functions. If C++ is
used, it is implemented as an overloaded wrapper function. If C is used, it is
implemented as a `_Generic` macro.

Because gcc doesn't disinguish between `const char *` and `const PROGMEM char *`
at compile-time, all constant strings are handled as program-space pointers by
`bang`. If you want to `bang` a constant string literal from RAM you need to
call `bang_str` directly, but consider wrapping it in `PSTR(...)` to put it
in program space instead and save memory.
**/

#ifdef __cplusplus

void bang(char chr) { bang_char(chr); }
void bang(unsigned char chr) { bang_char(chr); }
void bang(char *str) { bang_str(str); }
void bang(const char *str) { bang_pstr(str); }
#ifdef PRINTBANG_WANT_INT
void bang(int value, unsigned char base = 10) { bang_int(value, base); }
void bang(unsigned int value, unsigned char base = 10) { bang_uint(value, base); }
#endif
#ifdef PRINTBANG_WANT_LONG
void bang(long value, unsigned char base = 10) { bang_long(value, base); }
void bang(unsigned long value, unsigned char base = 10) { bang_ulong(value, base); }
#endif
#ifdef PRINTBANG_WANT_LONG_LONG
void bang(long long value, unsigned char base = 10) { bang_longlong(value, base); }
void bang(unsigned long long value, unsigned char base = 10) { bang_ulonglong(value, base); }
#endif
#ifdef PRINTBANG_WANT_FLOAT
void bang(float value, unsigned char places = 4) { bang_float(value, places); }
void bang(double value, unsigned char places = 4) { bang_float(value, places); }
#endif

#else // __cplusplus

// Signed types need to be listed before unsigned types 
#define bang(A, ...) _Generic((A), \
    char: bang_char, \
    unsigned char: bang_char, \
    char *: bang_str, \
    const char *: bang_pstr, \
    int: bang_int, \
    unsigned int: bang_uint, \
    long: bang_long, \
    unsigned long: bang_ulong, \
    long long: bang_longlong, \
    unsigned long long: bang_ulonglong, \
    float: bang_float, \
    double: bang_float, \
)(A __VA_OPT__(,) __VA_ARGS__)

#endif // __cplusplus

/**
#### `void bangln(...)` ([source]({anchor}))
This is a macro that first calls `bang` on the passed arguments and then
`bang_pstr` on `printbang_line_ending`.
**/
#define bangln(A, ...) do { \
    bang(A __VA_OPT__(,) __VA_ARGS__); \
    bang_pstr(printbang_line_ending); \
} while (0)

#endif // PRINTBANG_IMPLEMENTATION

#endif // PRINTBANG_H
