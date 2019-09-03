CC:=avr-gcc
OBJCOPY:=avr-objcopy
OBJDUMP:=avr-objdump
MICRONUCLEUS:=micronucleus

MCU?=attiny85
F_CPU?=16000000

INCLUDES=-I../..
DEFINES=-DF_CPU=$(F_CPU)

CFLAGS=$(INCLUDES) $(DEFINES) -mmcu=$(MCU) -Wall -g -ffunction-sections -fdata-sections -Os -flto
LINKFLAGS=-mmcu=$(MCU) -Wl,-gc-sections -flto
OBJCOPYFLAGS=-R .eeprom

OBJECTS:=\
main.o

OUTPUTS:=\
main.elf \
main.bin \
main.lst

%.hex: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) -O binary $< $@

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@


main.elf: $(OBJECTS)
	$(CC) $(LINKFLAGS) -o $@ $^

all: $(OUTPUTS)

micronucleus: main.hex
	$(MICRONUCLEUS) --type intel-hex --run $^

size: main.elf
	avr-size -C --mcu=$(MCU) $^

clean:
	$(RM) $(OBJECTS) $(OUTPUTS)
