CC = gcc

# Project Name
TARGET = program

# Generate dependency files (.d) for header tracking: -MMD -MP
# CFLAGS includes the GTK Header paths and warning flags
CFLAGS = -Wall -Wextra -g -MMD -MP $(shell pkg-config --cflags gtk4)

# LDFLAGS includes the GTK Library files
LDFLAGS = $(shell pkg-config --libs gtk4)

# Source files - removed duplicates (using Interface and Utils with capital letters)
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

# Main build target
program: $(OBJ)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run with: make run"

# Include generated dependency files
-include $(DEPS)

# Compilation rule
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

# Optional: Print variables for debugging
debug:
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "SRC: $(SRC)"
	@echo "OBJ: $(OBJ)"
	@echo "DEPS: $(DEPS)"