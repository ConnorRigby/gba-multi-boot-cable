# Makefile for 2ndloader, Matt'sSerialMultibootCable.
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

# devkitARM/arm-none-eabi/bin/ar
AS = $(DEVKITARM)/bin/arm-none-eabi-as
LD = $(DEVKITARM)/bin/arm-none-eabi-ld
OBJCOPY = $(DEVKITARM)/bin/arm-none-eabi-objcopy
SOURCES = loader.s
OBJS = loader.o

all: loader.h

clean:
	$(RM) *.o *.elf *.gba loader.h

loader.h: loader.gba
	./mkinc.pl < loader.gba > loader.h

loader.gba:	loader.elf
	$(OBJCOPY) -O binary loader.elf loader.gba

loader.elf:	$(OBJS)
	$(LD) -Ttext 0 -o loader.elf $(OBJS)

loader.o:	loader.s
	$(AS) -o loader.o loader.s
