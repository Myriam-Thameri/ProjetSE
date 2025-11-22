CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = main.c Config/config.c
OBJ = $(SRC:.c=.o)

program: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o program

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) program

run: program
	./program