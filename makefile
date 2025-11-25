# Nom de l'exécutable
TARGET = simulateur

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -I. -IConfig -IAlgorithms

# Fichiers sources
SRC = main.c Config/config.c Algorithms/fcfs.c

# Fichiers objets
OBJ = $(SRC:.c=.o)

# Règle principale
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Compilation des .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJ) $(TARGET)

