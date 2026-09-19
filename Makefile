CC := cc
CFLAGS := -std=c17 -Wall -Wextra -pedantic
APP := build/cobra

.PHONY: make run clean

make: $(APP)

$(APP): shell.c
	mkdir -p build
	$(CC) $(CFLAGS) shell.c -o $(APP)

run: $(APP)
	$(APP)

clean:
	rm -rf build
