CFLAGS = -Wall -Wextra
LDFLAGS = -lm
OBJ = buffer.o keybind.o main.o string.o terminal.o ui.o utilities.o

.PHONY: all
all: $(OBJ)
	$(CC) $(LDFLAGS) -o ked $(OBJ)

.PHONY: debug
debug: CFLAGS = -Wall -Wextra -O0 -g3
debug: all

.PHONY: clean
clean:
	$(RM) $(OBJ) ked
