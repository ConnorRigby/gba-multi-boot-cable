2NDLOADER = 2ndloader
GBA_MULTI_CABLE = gba-multi-cable

OBJS = serxfer.o main.o multi.o

CCOPTS = -g

all: gbl $(GBA_MULTI_CABLE)/.pio/build/nanoatmega328/firmware.hex

.PHONY: all clean burn

gbl: $(OBJS)
	gcc $(OBJS) -o gbl

main.o: main.c $(2NDLOADER)/loader.h

$(2NDLOADER)/loader.h:
	cd $(2NDLOADER) && make

$(GBA_MULTI_CABLE)/.pio/build/nanoatmega328/firmware.hex:
	pio run -d $(GBA_MULTI_CABLE)/ --environment nanoatmega328

burn: $(GBA_MULTI_CABLE)/.pio/build/nanoatmega328/firmware.hex
	pio run -d $(GBA_MULTI_CABLE)/ --environment nanoatmega328 -t upload

clean:	
	$(RM) *.o gbl
	cd $(2NDLOADER) && make clean
	pio run -d $(GBA_MULTI_CABLE)/ --environment nanoatmega328 -t clean