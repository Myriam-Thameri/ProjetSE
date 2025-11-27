#include "./Config/types.h"
#include "./Config/config.h"
#include "./Algorithms/Algorithms.h"

#include <stdio.h>
#include <stdlib.h>

char* PATH = "./Config/config.txt";

int main(void) {

    Config* CFG = malloc(sizeof(Config));

    if (!CFG) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    int config_load_res = load_config(PATH, CFG);

    if (config_load_res == 0) {
        printf("Error loading config file.\n");
        return 1;
    }

    printf("\n=== LOADED PROCESSES ===\n");
    for (int i = 0; i < CFG->process_count; i++) {
        PROCESS p = CFG->processes[i];
        printf("%s    Arr=%d    Exec=%d\n", p.ID, p.arrival_time, p.execution_time);
    }

    printf("\nRunning FCFS Algorithm...\n");
    FCFS_Algo(CFG);

    free(CFG);
    return 0;
}
