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

    /* ===== DISPLAY LOADED CONFIGURATION ===== */
    for (int i = 0; i < CFG->process_count; i++) {
        PROCESS p = CFG->processes[i];

        printf("\nProcess ID: %s\n", p.ID);
        printf("  Arrival Time: %d\n", p.arrival_time);
        printf("  Execution Time: %d\n", p.execution_time);
        printf("  Priority: %d\n", p.priority);
        printf("  IO Operations: %d\n", p.io_count);

        for (int j = 0; j < p.io_count; j++) {
            IO_OPERATION io = p.io_operations[j];
            printf("    IO #%d: Start=%d  Duration=%d\n",
                   j + 1, io.start_time, io.duration);
        }
    }

    printf("\nConfiguration loaded successfully.\n");

    /* ===== CALL FCFS SCHEDULER ===== */
    printf("\nRunning FCFS Scheduling Algorithm...\n");
    FCFS_Algo(CFG);

    free(CFG);

    return 0;
}
