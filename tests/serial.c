#include "serial.h"
#include <sim_avr.h>
#include <stdio.h>

DEFINE_FIFO(uint8_t, serial_buffer);

static int get_even_parity(uint8_t byte)
{
    uint8_t parity = 0;
    while (byte)
    {
        parity += byte & 1;
        byte >>= 1;
    }
    return !(parity & 1);
}

static void check_timing(avr_t *avr, serial_receiver *recv)
{
    int cycles = avr->cycle - recv->last_cycle;
    recv->last_cycle = avr->cycle;
    if (cycles != recv->expected_cycles)
    {
        if (recv->state == SERIAL_STATE_IN_WORD)
        {
            fprintf(
                stderr,
                "serial: Timing glitch before bit %u: expected %u cycles, counted %u\n",
                (recv->config.databits - recv->bits_remaining) + 1,
                recv->expected_cycles,
                cycles
            );
        }
        else
        {
            fprintf(
                stderr,
                "serial: Timing glitch before %s bit: expected %u cycles, counted %u\n",
                (recv->state == SERIAL_STATE_PARITY_BIT) ? "parity" : "stop",
                recv->expected_cycles,
                cycles
            );
        }
        recv->state = SERIAL_STATE_FAULT;
    }
}

static void serial_write_cb
(
	struct avr_t * avr,
    avr_io_addr_t addr,
    uint8_t value,
    void * param
)
{
    serial_receiver *recv = (serial_receiver *)(param);
    avr->data[addr] = value;

    uint8_t level = avr_regbit_get(avr, recv->regbit);

    int parity;
    switch (recv->state)
    {
        case SERIAL_STATE_IDLE:
            if (level == 0)
            {
                recv->state = SERIAL_STATE_IN_WORD;
                recv->bits_remaining = recv->config.databits;
                recv->current_byte = 0;
                recv->last_cycle = avr->cycle;
            }
            break;

        case SERIAL_STATE_IN_WORD:    
            recv->bits_remaining--;
            if (recv->config.order == SERIAL_ORDER_LSB)
            {
                recv->current_byte >>= 1;
                recv->current_byte |= level << 7;
            }
            else
            {
                recv->current_byte <<= 1;
                recv->current_byte |= level;
            }
            if (recv->bits_remaining == 0)
            {
                if (recv->config.parity != SERIAL_PARITY_NONE)
                    recv->state = SERIAL_STATE_PARITY_BIT;
                else
                    recv->state = SERIAL_STATE_STOP_BIT;
            }
            check_timing(avr, recv);
            break;

        case SERIAL_STATE_PARITY_BIT:
            parity = get_even_parity(recv->current_byte);
            if ((parity == level) != recv->config.parity)
            {
                fprintf(stderr, "serial: Wrong parity bit, expected %s parity\n",
                    (parity) ? "even" : "odd");
                recv->state = SERIAL_STATE_FAULT;
            }
            else
            {
                recv->state = SERIAL_STATE_STOP_BIT;
            }
            check_timing(avr, recv);
            break;

        case SERIAL_STATE_STOP_BIT:
            if (level == 1)
            {
                recv->state = SERIAL_STATE_IDLE;

                // Align byte to the right side
                if (recv->config.order == SERIAL_ORDER_LSB)
                    recv->current_byte >>= (8 - recv->config.databits);
                    
                serial_buffer_write(&recv->buffer, recv->current_byte);
            }
            check_timing(avr, recv);
            break;
    }
}

void serial_init(serial_receiver *recv, serial_config *config, avr_regbit_t regbit)
{
    recv->config = *config;
    recv->regbit = regbit;

    recv->current_byte = 0;
    recv->bits_remaining = recv->config.databits;
    recv->state = SERIAL_STATE_IDLE;
    
    serial_buffer_reset(&recv->buffer);
}

void serial_connect(avr_t *avr, serial_receiver *recv)
{
    recv->expected_cycles = avr->frequency / recv->config.baudrate;
    avr_register_io_write(avr, recv->regbit.reg, serial_write_cb, recv);
}

uint8_t serial_read(serial_receiver *recv)
{
    return serial_buffer_read(&recv->buffer);
}

int serial_available(serial_receiver *recv)
{
    return !serial_buffer_isempty(&recv->buffer);
}
