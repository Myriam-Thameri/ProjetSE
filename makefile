CC = gcc

TARGET = program

CFLAGS = -Wall -Wextra -g -MMD -MP $(shell pkg-config --cflags gtk4)

LDFLAGS = $(shell pkg-config --libs gtk4)

SRC = main.c \
	Config/config.c \
	$(wildcard Algorithms/*.c) \
	Utils/Algorithms.c \
	Interface/interface.c \
	Utils/utils.c \
	Interface/gantt_chart.c \
	Utils/log_file.c

OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.d)

program: $(OBJ)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run with: make run"

-include $(DEPS)

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean run all

all: clean $(TARGET)

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJ) $(DEPS) $(TARGET)
	@echo "Clean complete!"

run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

debug:
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "DEPS: $(DEPS)"