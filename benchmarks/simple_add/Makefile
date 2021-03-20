src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -Wall -Wextra
LDFLAGS = -lm

simple_add: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) simple_add