CC = gcc
CFLAGS = -Wall -Os -march=native -mtune=native -I.
OPENMP = -fopenmp
LDFLAGS = 
SRCS = perf.c redfst_omp.c init.c region.c global.c util.c profile.c monitor.c energy.c msr.c control.c
OBJS = $(SRCS:.c=.o)
TARGET = libredfst.so
.PHONY: all clean install
all: $(SRCS) $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OPENMP) -shared $(OBJS) $(LDFLAGS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) $(OPENMP) -fPIC -c $<

install: $(TARGET)
	install -m 644 -D libredfst.so $(PREFIX)/lib/libredfst.so
	install -m 644 -D redfst.h $(PREFIX)/include/redfst.h
	for x in redfst/*; do \
		install -m 644 -D $$x ${PREFIX}/include/$$x; \
	done

clean:
	rm -fr $(OBJS) $(TARGET)
