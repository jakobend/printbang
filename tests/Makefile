CC=gcc
INCLUDES=-I./acutest -I/usr/include/simavr
LINKFLAGS=-lsimavr -lelf

CFLAGS=$(INCLUDES) -Wall -O3 -g

OBJECTS:=\
serial.o \
runner.o
OUTPUTS=runner

firmware/firmware.elf:
	$(MAKE) -C ./firmware

firmware: firmware/firmware.elf

runner: $(OBJECTS)
	$(CC) $(LINKFLAGS) $^ -o $@

all: $(OUTPUTS) firmware

clean:
	$(RM) $(OBJECTS)
	$(RM) $(OUTPUTS)
	$(MAKE) -C ./firmware clean
