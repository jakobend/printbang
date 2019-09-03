#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#include <sim_avr.h>

DECLARE_FIFO(uint8_t, serial_buffer, 256);

typedef enum serial_parity
{
    SERIAL_PARITY_NONE = -1,
    SERIAL_PARITY_EVEN = 0,
    SERIAL_PARITY_ODD = 1
} serial_parity;

typedef enum serial_order
{
    SERIAL_ORDER_LSB,
    SERIAL_ORDER_MSB
} serial_order;

typedef enum serial_state
{
    SERIAL_STATE_IDLE,
    SERIAL_STATE_IN_WORD,
    SERIAL_STATE_STOP_BIT,
    SERIAL_STATE_PARITY_BIT,
    SERIAL_STATE_FAULT
} serial_state;

typedef struct serial_config
{
    uint32_t baudrate;
    uint8_t databits;
    serial_parity parity;
    serial_order order;
} serial_config;

typedef struct serial_receiver
{
    serial_config config;
    avr_regbit_t regbit;
    serial_buffer_t buffer;
    uint8_t current_byte;
    uint8_t bits_remaining;

    serial_state state;

    avr_cycle_count_t last_cycle;
    int expected_cycles;
} serial_receiver;

void serial_init(serial_receiver *recv, serial_config *config, avr_regbit_t regbit);
void serial_connect(avr_t *avr, serial_receiver *recv);
uint8_t serial_read(serial_receiver *recv);
int serial_available(serial_receiver *recv);

#endif
