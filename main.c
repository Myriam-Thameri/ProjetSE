#include <stdio.h>
#include "config.h"

// declare your scheduler function
void run_priority_preemptive(PROCESS p[], int count);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    PROCESS processes[MAX_PROCESSES];
    int count = 0;

    // load configuration
    if (load_config(argv[1], processes, &count) != 0) {
        printf("Failed to load configuration.\n");
        return 1;
    }

    printf("Loaded %d processes.\n", count);

    run_priority_preemptive(processes, count);

    return 0;
}
