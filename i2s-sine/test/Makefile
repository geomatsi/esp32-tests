#

VPATH += ../main

SRCS := test.c sine_float.c sine_cordic16.c sine_cordic32.c
OBJS := $(SRCS:.c=.o)

all: test

test: $(OBJS)
	$(CC) $^ -g -o $@ -lm

graph: test
	./test.sh

%.o: %.c
	$(CC) $(OPTS) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf *.o
	rm -rf test
	rm -rf sine.dat 

.PHONY: all clean
