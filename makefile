CC = gcc
# Generate dependency files (.d) for header tracking: -MMD -MP
CFLAGS = -Wall -Wextra -g -MMD -MP

# Add GTK4 flags
GTK_FLAGS = $(shell pkg-config --cflags --libs gtk4)

SRC = \
Config/config.c \
Algorithms/SJF.c \
Algorithms/RoundRobin.c \
Algorithms/srt.c \
Algorithms/MultilevelStatic.c \
Algorithms/fcfs.c \
Algorithms/multilevel_aging.c \
Algorithms/PreemptivePriority.c \
Algorithms/Algorithms.c \
Interface/Interface.c \
Interface/gantt_chart.c \
Interface/interface_utils.c \
get_algorithms_names.c

OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.d)

program: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o program $(GTK_FLAGS)

# Include generated dependency files (ignore missing ones on first run)
-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) $(GTK_FLAGS) -c $< -o $@

.PHONY: clean run

clean:
	rm -f $(OBJ) $(DEPS) program

run: program
	./program
