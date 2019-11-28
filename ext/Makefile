.PHONY: all
all:
	$(MAKE) -C global_keybind $(SUBMAKE_TARGET)

.PHONY: debug
debug: SUBMAKE_TARGET = debug
debug: all

.PHONY: clean
clean:
	$(MAKE) -C global_keybind clean
