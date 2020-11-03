CFLAGS= -O3 -fno-stack-protector
SRCS=$(wildcard *.c) $(wildcard */*.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c $(CFLAGS) $< -o $@

all: $(OBJS)
	gcc $(CFLAGS) $(OBJS) -lrt -lpthread -o sys-mon

clean:
	rm -f *.o a.out */*.o