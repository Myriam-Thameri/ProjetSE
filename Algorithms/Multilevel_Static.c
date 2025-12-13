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
    clear_io_slices();
    
    PCB* pcbs = initialize_PCB(config);
    int time = 0;
    int finished_processes = 0;
    int total_processes = config->process_count;

    printf("\n=== Multilevel Static Scheduler Start ===\n");

    while (finished_processes < total_processes) {

        // Process I/O operations - decrement timers only, don't add slices here
        for (int i = 0; i < total_processes; i++) {
            if (pcbs[i].in_io && !pcbs[i].finished) {
                pcbs[i].io_remaining--;
                
                if (pcbs[i].io_remaining <= 0) {
                    pcbs[i].in_io = 0;
                    printf("Time %d: %s completes I/O\n", time, pcbs[i].process.ID);
                    log_print("Time %d: %s completes I/O\n", time, pcbs[i].process.ID);
                }
            }
        }

        // Select next process
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
            // Execute for 1 time unit to keep CPU and I/O in sync
            printf("Time %d: |%-4s (Priority %d)\n",
                   time,
                   next->process.ID,
                   next->process.priority);
            
            log_print("Time %d: |%-4s\n", time, next->process.ID);
        
            add_gantt_slice(next->process.ID, time, 1, NULL);

            next->remaining_time--;
            next->executed_time++;

            // Check for I/O after this execution
            if (next->io_index < next->process.io_count) {
                IO_OPERATION *io_op = &next->process.io_operations[next->io_index];
                if (next->executed_time >= io_op->start_time && next->remaining_time > 0) {
                    next->io_index++;
                    next->io_remaining = io_op->duration;
                    next->in_io = 1;
                    
                    // ADD THE FULL I/O SLICE HERE (once, with full duration)
                    add_io_slice(next->process.ID, time + 1, io_op->duration, NULL, "I/O");
                    
                    printf("  -> %s will start I/O at time %d for %d units\n", 
                           next->process.ID, time + 1, io_op->duration);
                    log_print("  -> %s enters I/O for %d units\n", 
                             next->process.ID, io_op->duration);
                }
            }

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished_processes++;
                printf("Process %s finished at time %d\n",
                       next->process.ID, time + 1);
                log_print("Process %s finished at time %d\n",
                       next->process.ID, time + 1);
            }
        } else {
            printf("Time %d: CPU idle\n", time);
            log_print("Time %d: CPU idle\n", time);
            add_gantt_slice("IDLE", time, 1, "#cccccc");
        }
        
        time++;
    }

    printf("=== Multilevel Static Scheduler End ===\n");
}