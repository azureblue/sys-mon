CFLAGS= -O3 -fno-stack-protector
SRCS=$(wildcard *.c) $(wildcard modules/*.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c $(CFLAGS) $< -o $@

all: $(OBJS)
	gcc $(CFLAGS) $(OBJS) -lrt -lpthread -o sys-mon

simple-client:
	gcc $(CFLAGS) client/simple-client.c -lrt -lpthread -o simple-client

clean:
	rm -f *.o a.out */*.o simple-client sys-mon