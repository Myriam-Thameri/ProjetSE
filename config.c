#include <stdio.h>
#include <stdlib.h>
#include "config.h"

int load_config(const char *filename, PROCESS p[], int *count) {
    FILE *f = fopen("config.txt", "r");
    if (!f) {
        printf("Error: Cannot open config file: %s\n", filename);
        return -1;
    }

    int i = 0;
    while (i < MAX_PROCESSES) {
        PROCESS proc;

        // Try to read basic process info
        int base = fscanf(
            f, "%d %d %d %d %d",
            &proc.id,
            &proc.arrival_time,
            &proc.execution_time,
            &proc.priority,
            &proc.io_count
        );

        if (base == EOF)
            break;

        if (base < 5) {
            printf("Error: Invalid format in config file (process header).\n");
            fclose(f);
            return -1;
        }

        if (proc.io_count > MAX_IO_PER_PROCESS) {
            printf("Error: Process %d has too many I/O operations (%d).\n",
                   proc.id, proc.io_count);
            fclose(f);
            return -1;
        }

        // Read all IO operations for this process
        for (int j = 0; j < proc.io_count; j++) {
            int ret = fscanf(
                f, "%d %d",
                &proc.io_operations[j].start_time,
                &proc.io_operations[j].duration
            );

            if (ret < 2) {
                printf("Error: Invalid IO entry for process %d\n", proc.id);
                fclose(f);
                return -1;
            }
        }

        p[i++] = proc;
    }

    *count = i;
    fclose(f);
    return 0;
}

