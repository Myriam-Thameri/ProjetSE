#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void MultilevelStaticScheduler(Config* config) {

    int QUANTUM;
    printf("Enter the quantum value: ");
    scanf("%d", &QUANTUM);

    if (QUANTUM <= 0) {
        printf("Invalid quantum. Using default quantum = 2\n");
        QUANTUM = 2;
    }

    PCB* pcbs = initialize_PCB(config);
    int time = 0;
    int finished_processes = 0;
    int total_processes = config->process_count;

    printf("\n=== Multilevel Static Scheduler Start ===\n");

    
    while (finished_processes < total_processes) {

        PCB* next = NULL;

      
        for (int i = 0; i < total_processes; i++) {
            if (!pcbs[i].finished &&
                !pcbs[i].in_io &&
                pcbs[i].process.arrival_time <= time) {

                if (!next || pcbs[i].process.priority > next->process.priority) {
                    next = &pcbs[i];
                }
            }
        }

        if (next) {
            int run_time = QUANTUM;
            if (next->remaining_time < run_time)
                run_time = next->remaining_time;

            printf("Time %d-%d: |%-4s (Priority %d)\n",
                   time,
                   time + run_time,
                   next->process.ID,
                   next->process.priority);

            next->remaining_time -= run_time;
            time += run_time;

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished_processes++;
                printf("Process %s finished at time %d\n",
                       next->process.ID, time);
            }
        } else {
            printf("Time %d: CPU idle\n", time);
            time++;
        }
    }

    printf("=== Multilevel Static Scheduler End ===\n");
}