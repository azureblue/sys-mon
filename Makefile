CFLAGS= -O3 -flto -std=c11  -fno-stack-protector -fvisibility=hidden
#  CFLAGS= -g -std=c11  -fno-stack-protector

SRCS=$(wildcard *.c) $(wildcard modules/*.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -rdynamic $(CFLAGS) $< -o $@

%.lo : %.c
	gcc -fPIC -c -rdynamic $(CFLAGS) $< -o $@

all: sys-mon genmon

sys-mon: $(OBJS) simple-client
	gcc $(CFLAGS) --whole-file -rdynamic $(OBJS) -lrt -lpthread -o sys-mon
	strip -sx sys-mon

client.a: client/client.lo
	ar rcs client.a client/client.lo

simple-client: client.a
	gcc $(CFLAGS) client/simple-client.c client.a -lrt -lpthread -o simple-client

genmon:
	+ @make -C genmon

clean:
	+ @make -C genmon clean
	rm -f *.o a.out */*.o */*.a */*.lo simple-client sys-mon client.a

.PHONY: genmon clean all