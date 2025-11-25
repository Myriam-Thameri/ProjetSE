#include <stdio.h>
#include "Config/config.h"
#include "Algorithms/fcfs.h" // déclare simulate_fcfs_with_io

int main(void) {
    Config cfg;
    if (load_config("Config/config.txt", &cfg)) {
        simulate_fcfs_with_io(&cfg);
    } else {
        printf("Erreur de chargement du fichier de configuration.\n");
    }
    return 0;
}
