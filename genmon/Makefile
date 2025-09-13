#CFLAGS= -g  -std=c11  -fno-stack-protector -fvisibility=hidden
CFLAGS= -O3 -flto -std=c11  -fno-stack-protector -fvisibility=hidden

FOREIGN_OBJS= ../client.a $(filter-out ../sys-mon.o, $(wildcard ../*.o))
MODULES_OBJS=$(wildcard ../modules/*.o)

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

%.o : %.c
	gcc -c -rdynamic $(CFLAGS) $< -o $@

all: $(FOREIGN_OBJS) genmon sys-mon-pango.so sys-mon-pango-tooltip.so my-system-info.so

genmon: genmon.o
	gcc $(CFLAGS) genmon.o -o genmon -ldl
	# strip -sx genmon

sys-mon-pango-tooltip.so: sys-mon-pango-tooltip.o ../writter.o block-bars-utils.o
	gcc $(CFLAGS)  sys-mon-pango-tooltip.o ../writter.o block-bars-utils.o -fvisibility=hidden  -rdynamic  -fPIC -shared -o sys-mon-pango-tooltip.so
	# strip -sx sys-mon-pango-tooltip.so

cpu-stats: cpu-stats.o
	gcc $(CFLAGS)  cpu-stats.o -fvisibility=hidden --whole-file -rdynamic $(MODULES_OBJS) $(FOREIGN_OBJS) -o cpu-stats

my-system-info.so: my-system-info.o
	gcc $(CFLAGS)  my-system-info.o -fvisibility=hidden --whole-file -rdynamic $(MODULES_OBJS) $(FOREIGN_OBJS) -fPIC -shared -o my-system-info.so
	strip -sx my-system-info.so

sys-mon-pango.so: sys-mon-pango.o
	gcc $(CFLAGS)  sys-mon-pango.o -fvisibility=hidden --whole-file -rdynamic $(MODULES_OBJS) $(FOREIGN_OBJS) -fPIC -shared -o sys-mon-pango.so
	strip -sx sys-mon-pango.so

beep: $(OBJS_BEEP)
	gcc $(CFLAGS) $(OBJS_BEEP) -o beep
	sudo chown root beep
	sudo chmod 4775 beep

clean:
	rm -f *.o *.so a.out */*.o */*.a */*.lo */*.so genmon
