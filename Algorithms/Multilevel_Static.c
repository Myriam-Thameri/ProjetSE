/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Config/types.h"
#include "../Config/config.h"
#include "../Utils/Algorithms.h"
#include "../Interface/gantt_chart.h"
#include "../Utils/log_file.h"


void MultilevelStaticScheduler(Config* config, int quantum) {
    clear_gantt_slices();
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
            int run_time = quantum;
            if (next->remaining_time < run_time)
                run_time = next->remaining_time;

            printf("Time %d-%d: |%-4s (Priority %d)\n",
                   time,
                   time + run_time,
                   next->process.ID,
                   next->process.priority);
            
            log_print("Time %d-%d: |%-4s", time, time+run_time, next->process.ID);
        
            add_gantt_slice(next->process.ID, time, run_time, NULL);

            next->remaining_time -= run_time;
            time += run_time;

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished_processes++;
                printf("Process %s finished at time %d\n",
                       next->process.ID, time);
                log_print("Process %s finished at time %d\n",
                       next->process.ID, time);
            }
        } else {
            printf("Time %d: CPU idle\n", time);
            log_print("Time %d: CPU idle\n", time);
            add_gantt_slice("IDLE", time, 1, "#cccccc");
            time++;
        }
    }

    printf("=== Multilevel Static Scheduler End ===\n");
}
