CFLAGS = -Wall -Wextra
OBJS = editcommand.o

keybind.so: $(OBJS)
	$(CC) $(OBJS) -o keybind.so -shared

.PHONY: debug
debug: CFLAGS = -Wall -Wextra -O0 -g3
debug: keybind.so

.PHONY: clean
clean:
	$(RM) $(OBJS) keybind.so
