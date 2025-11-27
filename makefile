# Nom de l'exécutable
TARGET = simulateur

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -I. -IConfig -IAlgorithms

# Fichiers sources
SRC = main.c \
      Config/config.c \
      Algorithms/fcfs.c

# Fichiers objets
OBJ = main.o \
      Config/config.o \
      Algorithms/fcfs.o

# ===========================
#        RÈGLE PRINCIPALE
# ===========================
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# ===========================
#   RÈGLES DE COMPILATION
# ===========================

main.o: main.c Config/config.h Config/types.h Algorithms/Algorithms.h
	$(CC) $(CFLAGS) -c main.c -o main.o

Config/config.o: Config/config.c Config/config.h Config/types.h
	$(CC) $(CFLAGS) -c Config/config.c -o Config/config.o

Algorithms/fcfs.o: Algorithms/fcfs.c Algorithms/Algorithms.h Config/config.h Config/types.h
	$(CC) $(CFLAGS) -c Algorithms/fcfs.c -o Algorithms/fcfs.o

# ===========================
#          CLEAN
# ===========================
clean:
	rm -f main.o Config/config.o Algorithms/fcfs.o $(TARGET)
