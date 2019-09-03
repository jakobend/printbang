CC=avr-gcc
OBJDUMP=avr-objdump

MCU?=attiny85
F_CPU?=16000000

OBJECTS=firmware.o
OUTPUT_PREFIX=firmware

OUTPUTS=$(addprefix $(OUTPUT_PREFIX), .elf .lst .map)

INCLUDES:=-I../.. -I/usr/include/simavr/avr
DEFINES:=-DF_CPU=$(F_CPU) -DMCU=\"$(MCU)\"

CFLAGS:=-mmcu=$(MCU) -g -gstabs -Wall -Os \
$(INCLUDES) $(DEFINES)

LINKFLAGS:=-mmcu=$(MCU) \
-Wl,--undefined=_mmcu,--section-start=.mmcu=0x910000 \
-Wl,-gc-sections \
-Wl,-print-gc-sections \
-Wl,-Map,$(OUTPUT_PREFIX).map

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

$(OUTPUT_PREFIX).elf: $(OBJECTS)
	$(CC) $(LINKFLAGS) -o $@ $^

all: $(OUTPUTS)

clean:
	$(RM) $(OBJECTS)
	$(RM) $(OUTPUTS)
