


GCC := gcc
PROGRAMMER := programmer
ASM := clock2

PROGRAMMER_OBJS := \
	programmer.o \
	parapin.o

all: $(PROGRAMMER) $(ASM)


temp_log: temp_log.c
	$(GCC) -g -o temp_log temp_log.c -lpthread -lrt

miditoclock: miditoclock.c
	$(GCC) -g -o miditoclock miditoclock.c

testclock: testclock.c
	$(GCC) -g -o testclock testclock.c

$(PROGRAMMER): $(PROGRAMMER_OBJS)
	$(GCC) -o $(PROGRAMMER) $(PROGRAMMER_OBJS) -lpthread

$(ASM): thermotable.inc $(ASM).s
	gpasm -p 16f877a -o $(ASM).hex $(ASM).s


$(PROGRAMMER_OBJS):
	$(GCC) -c -O2 $*.c -o $*.o

thermotable.inc: thermotable.c
	$(GCC) thermotable.c -o thermotable
	thermotable > thermotable.inc

dist:
        miditoclock.c
        

clean:
	rm -f thermotable thermotable.inc *.o *.s19 executor programmer 
	


executor.o: executor.c
lib68hc11e1.o: lib68hc11e1.c
parapin.o: parapin.c
programmer.o: programmer.c
clock2.hex: clock2.s

