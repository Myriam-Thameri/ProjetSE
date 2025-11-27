#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUANTUM 2


void MultilevelStaticScheduler(Config* config) {
    PCB* pcbs = initialize_PCB(config); // do NOT free
    int time = 0;
    int finished_processes = 0;
    int total_processes = config->process_count;
    
    printf("=== Multilevel Static Scheduler Start ===\n");
    
    // Main loop
    while (finished_processes < total_processes) {
        PCB* next = NULL;
        
        // Pick highest priority ready process
        for (int i = 0; i < total_processes; i++) {
            if (!pcbs[i].finished && !pcbs[i].in_io && pcbs[i].process.arrival_time <= time) {
                if (!next || pcbs[i].process.priority > next->process.priority) {
                    next = &pcbs[i];
                }
            }
        }
        
        if (next) {
            int run_time = 2; // RR quantum
            if (next->remaining_time < run_time) run_time = next->remaining_time;
            printf("Time %d-%d: |%-4s", time, time+run_time, next->process.ID);
            next->remaining_time -= run_time;
            time += run_time;
            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished_processes++;
            }
        } else {
            printf("Time %d: CPU idle\n", time);
            time++;
        }
    }
    
    printf("\n=== Multilevel Static Scheduler End ===\n");
}