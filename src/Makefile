AR = ar
CC = gcc
CFLAGS = -Wall -Os -march=native -mtune=native -I.
OPENMP = -fopenmp
LDFLAGS = 
SRCS = perf.c redfst_omp.c init.c region.c global.c util.c profile.c monitor.c energy.c msr.c control.c
OBJS = $(SRCS:.c=.o)
TARGET = libredfst.so #libredfst.a
.PHONY: all clean install test
all: $(SRCS) $(OBJS) $(TARGET)

libredfst.so : $(OBJS)
	$(CC) $(CFLAGS) $(OPENMP) -shared $(OBJS) $(LDFLAGS) -o $@

# currently broken
#libredfst.a : $(OBJS)
#	 $(AR) rcs $@ $(OBJS)

%.o : %.c
	$(CC) $(CFLAGS) $(OPENMP) -fPIC -c $<

install: $(TARGET)
	install -m 644 -D libredfst.so $(PREFIX)/lib/libredfst.so
	install -m 644 -D redfst.h $(PREFIX)/include/redfst.h
	for x in redfst/*; do \
		install -m 644 -D $$x ${PREFIX}/include/$$x; \
	done

test0: libredfst.so test0.c
	if ld -lcpupower 2> /dev/null; then\
		$(CC) $(CFLAGS) -Wl,-rpath="`pwd`" test0.c -o $@ -L. -lredfst -lcpupower -lm;\
	else\
		$(CC) $(CFLAGS) -Wl,-rpath="`pwd`" test0.c -o $@ -L. -lredfst -lcpufreq  -lm;\
	fi

# this only tests the timing
# energy readings are not validated
test: test0
	env OMP_PROC_BIND=TRUE ./tester ./test0 < test0.answer
	echo All tests passed

clean:
	rm -fr $(OBJS) $(TARGET) test0
