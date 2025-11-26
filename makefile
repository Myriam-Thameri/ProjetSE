CC = gcc
# Generate dependency files (.d) for header tracking: -MMD -MP
CFLAGS = -Wall -Wextra -g -MMD -MP

SRC = main.c Config/config.c Algorithms/RoundRobin.c Algorithms/MultilevelStatic.c
OBJ = $(SRC:.c=.o)
DEPS = $(SRC:.c=.d)

program: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o program

# Include generated dependency files (ignore missing ones on first run)
-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean run

clean:
	rm -f $(OBJ) $(DEPS) program

run: program
	./program
