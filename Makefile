CFLAGS = -Wall -Wextra
LDFLAGS = -lm
OBJ = main.o terminal.o ui.o utilities.o

.PHONY: all
all: $(OBJ)
	$(CC) $(LDFLAGS) -o ked $(OBJ)

.PHONY: clean
clean:
	$(RM) $(OBJ) ked
