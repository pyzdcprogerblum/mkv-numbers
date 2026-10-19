CC = clang
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
ARGS =

.PHONY: all run clean

all: mkv_numbers

mkv_numbers: main.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c $(LDFLAGS) -o $@

run: mkv_numbers
	./mkv_numbers $(ARGS)

clean:
	rm -f mkv_numbers
