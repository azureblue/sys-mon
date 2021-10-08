CFLAGS= -O3 -flto -std=c11  -fno-stack-protector -fvisibility=hidden
FOREIGN_OBJS= ../client.a $(filter-out ../sys-mon.o, $(wildcard ../*.o))
MODULES_OBJS=$(wildcard ../modules/*.o)


SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -rdynamic $(CFLAGS) $< -o $@

all: genmon sys-mon-pango.so

genmon: genmon.o
	gcc $(CFLAGS) genmon.o -o genmon -ldl
	strip -sx genmon

sys-mon-pango.so: sys-mon-pango.o
	gcc $(CFLAGS)  sys-mon-pango.o -fvisibility=hidden --whole-file -rdynamic $(MODULES_OBJS) $(FOREIGN_OBJS) -fPIC -shared -o sys-mon-pango.so
	strip -sx sys-mon-pango.so

beep: $(OBJS_BEEP)
	gcc $(CFLAGS) $(OBJS_BEEP) -o beep
	sudo chown root beep
	sudo chmod 4775 beep

clean:
	rm -f *.o *.so a.out */*.o */*.a */*.lo */*.so genmon
