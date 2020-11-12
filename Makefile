#CFLAGS= -O3 -std=c11  -fno-stack-protector
CFLAGS= -g -std=c11  -fno-stack-protector

SRCS=$(wildcard *.c) $(wildcard modules/*.c)
OBJS=$(SRCS:.c=.o)

OBJS_GENMON=genmon/genmon.o read_buffer.o writter.o
OBJS_BEEP=genmon/beep.o

%.o : %.c
	gcc -c -D_POSIX_C_SOURCE=200112L -rdynamic $(CFLAGS) $< -o $@

%.lo : %.c
	gcc -fPIC -c -D_POSIX_C_SOURCE=200112L -rdynamic $(CFLAGS) $< -o $@

sys-mon: $(OBJS)
	gcc $(CFLAGS) --whole-file -rdynamic $(OBJS) -lrt -lpthread -ldl -o sys-mon
	#strip sys-mon

client: client/client.lo
	ar rcs client.a client/client.lo

simple-client: client/simple-client.o client
	gcc $(CFLAGS) client/simple-client.c client.a -lrt -lpthread -o simple-client

genmon: $(OBJS_GENMON)
	gcc $(CFLAGS) $(OBJS_GENMON) client.a -o genmon/genmon -lrt -lpthread

beep: $(OBJS_BEEP)
	gcc $(CFLAGS) $(OBJS_BEEP) -o genmon/beep
	sudo chown root genmon/beep
	sudo chmod 4775 genmon/beep

clean:
	rm -f *.o a.out */*.o simple-client sys-mon genmon/genmon
