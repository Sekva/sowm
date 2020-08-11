CFLAGS += -std=c99 -Wall -Wextra -pedantic -Wold-style-declaration
CFLAGS += -Wmissing-prototypes -Wno-unused-parameter
CC     ?= clang

all: sowm

config.h:
	cp config.def.h config.h

sowm: sowm.c sowm.h config.h Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< -lX11 $(LDFLAGS)

clean:
	rm -f sowm *.o config.h

test: all
	Xephyr -br -ac -noreset -screen 1200x720 :1 &
	sleep 0.1s
	DISPLAY=:1 ./sowm &
	sleep 0.1s
	DISPLAY=:1 xterm; pkill sowm; pkill Xephyr
	

.PHONY: all clean
