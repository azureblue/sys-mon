CFLAGS= -O3 -flto -std=c11  -fno-stack-protector
FOREIGN_OBJS= ../client.a ../writter.o

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -rdynamic $(CFLAGS) $< -o $@

all: genmon sys-mon-pango.so

genmon: genmon.o sys-mon-pango.o
	gcc $(CFLAGS) genmon.o sys-mon-pango.o $(FOREIGN_OBJS) -o genmon -lrt -lpthread
	strip -sx genmon

sys-mon-pango.so: sys-mon-pango.o
	gcc $(CFLAGS) -fvisibility=hidden  sys-mon-pango.o $(FOREIGN_OBJS) -fPIC -shared -o sys-mon-pango.so -lrt -lpthread

beep: $(OBJS_BEEP)
	gcc $(CFLAGS) $(OBJS_BEEP) -o beep
	sudo chown root beep
	sudo chmod 4775 beep

clean:
	rm -f *.o a.out */*.o */*.a */*.lo */*.so genmon
