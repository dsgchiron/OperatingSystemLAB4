CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -O0

PROGRAMS = part3-replace/save

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean
