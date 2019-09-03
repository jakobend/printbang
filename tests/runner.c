#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>

#include <sim_avr.h>
#include <sim_elf.h>
#include <avr_ioport.h>

#include "serial.h"

avr_t *avr = NULL;
serial_receiver recv;

serial_config conf = {
    .baudrate = 250000,
    .databits = 7,
    .parity = SERIAL_PARITY_NONE,
    .order = SERIAL_ORDER_LSB
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s FIRMWARE\n", argv[0]);
        return 1;
    }

    elf_firmware_t firmware;
    printf("Loading firmware from %s\n", argv[1]);
    if (elf_read_firmware(argv[1], &firmware))
    {
        fprintf(stderr, "%s: Could not read firmware\n", argv[0]);
        return 1;
    }

    printf ("firmware %s f=%d mmcu=%s\n", basename(argv[1]), (int) firmware.frequency, firmware.mmcu);
    avr = avr_make_mcu_by_name(firmware.mmcu);
    if (!avr)
    {
        fprintf(stderr, "%s: AVR '%s' not know\n", argv[0], firmware.mmcu);
        return 1;
    }

    avr_init(avr);
    avr_load_firmware(avr, &firmware);

    avr_io_addr_t port_addr = 0x18 + 0x20;
    serial_init(&recv, &conf, (avr_regbit_t)AVR_IO_REGBIT(port_addr, 0));
    serial_connect(avr, &recv);

    int state = cpu_Running;
    while ((state != cpu_Done) && (state != cpu_Crashed))
    {
        state = avr_run(avr);
        if (serial_available(&recv))
        {
            putchar((char)(serial_read(&recv)));
        }
    }
}
