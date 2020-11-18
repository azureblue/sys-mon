CFLAGS= -O3 -flto -std=c11  -fno-stack-protector
FOREIGN_OBJS= ../client.a ../writter.o

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -rdynamic $(CFLAGS) $< -o $@

all: genmon genmon2

genmon: genmon.o
	gcc $(CFLAGS) genmon.o $(FOREIGN_OBJS) -o genmon -lrt -lpthread
	strip -sx genmon

genmon2: genmon2.o
	gcc $(CFLAGS) genmon2.o $(FOREIGN_OBJS) -o genmon2 -lrt -lpthread
	strip -sx genmon2

beep: $(OBJS_BEEP)
	gcc $(CFLAGS) $(OBJS_BEEP) -o beep
	sudo chown root beep
	sudo chmod 4775 beep

clean:
	rm -f *.o a.out */*.o */*.a */*.lo genmon genmon2
