AR = ar
CC = gcc
CFLAGS = -Wall -Os -march=native -mtune=native -I.
OPENMP = -fopenmp
LDFLAGS = 
SRCS = perf.c redfst_omp.c init.c region.c region-all.c global.c profile.c monitor.c energy.c msr.c likwid.c control.c hw.c trace.c util.c
OBJS = $(SRCS:.c=.o)
TARGET = libredfst.so #libredfst.a
TESTS = test0 test1
.PHONY: all clean install test
all: $(SRCS) $(OBJS) $(TARGET)

libredfst.so : $(OBJS)
	$(CC) $(CFLAGS) $(OPENMP) -shared $(OBJS) $(LDFLAGS) -o $@

# currently broken
#libredfst.a : $(OBJS)
#	 $(AR) rcs $@ $(OBJS)

redfst-static.h: util.c control.c region.c region-all.h
	if $(CC) $(CFLAGS) -dM -E config.h | grep REDFST_FUN_IN_H > /dev/null; then\
		sh headerfun > $@;\
	fi

%.o : %.c redfst-static.h
	$(CC) $(CFLAGS) $(OPENMP) -fPIC -c $<

install: $(TARGET)
	install -m 644 -D libredfst.so "$(DESTDIR)/$(PREFIX)/lib/libredfst.so"
	if $(CC) $(CFLAGS) -dM -E config.h | grep REDFST_FUN_IN_H > /dev/null; then\
		install -m 644 -D redfst-static.h "$(DESTDIR)/$(PREFIX)/include/redfst.h";\
	else\
		install -m 644 -D redfst-default.h "$(DESTDIR)/$(PREFIX)/include/redfst.h";\
	fi

$(TESTS): %: %.c libredfst.so
	if ld -lcpupower 2> /dev/null; then\
		$(CC) $(CFLAGS) -Wl,-rpath="`pwd`" $< -o $@ -I. -L. -lredfst -lcpupower -lm;\
	else\
		$(CC) $(CFLAGS) -Wl,-rpath="`pwd`" $< -o $@ -I. -L. -lredfst -lcpufreq  -lm;\
	fi

# test0 tests the timing
# energy readings are not validated
# test1 tests frequency scaling
test: $(TESTS)
	env OMP_PROC_BIND=TRUE ./tester ./test0 < test0.answer
	rm test1.answer 2> /dev/null || true;\
	i=0;\
	for x in $$(cat $$(find /sys 2>&-|grep scaling_available_frequencies | head -n1) | sort -n); do\
		export REDFST_$$x=$$i;\
		echo $$x >> test1.answer;\
		i=$$(( i + 1 ));\
	done;\
	for i in $$(seq $$i 63); do\
		echo $$x >> test1.answer;\
	done;\
	env OMP_PROC_BIND=TRUE ./test1 > test1.out
	diff test1.out test1.answer
	echo All tests passed

clean:
	rm -fr $(OBJS) $(TARGET) $(TESTS)
