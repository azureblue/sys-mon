CFLAGS= -std=c11 -g -fno-stack-protector
SRCS=$(wildcard *.c) $(wildcard modules/*.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -D_POSIX_C_SOURCE=200112L -rdynamic $(CFLAGS) $< -o $@

all: $(OBJS) simple-client
	gcc $(CFLAGS) --whole-file -rdynamic $(OBJS) -lrt -lpthread -ldl -o sys-mon
	strip sys-mon

simple-client:
	gcc $(CFLAGS) client/simple-client.c -lrt -lpthread -o simple-client

clean:
	rm -f *.o a.out */*.o simple-client sys-mon
