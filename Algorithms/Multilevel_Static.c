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
    

    int current_quantum_used = 0;
    PCB* last_executed = NULL;

    printf("\n=== Multilevel Static Scheduler Start (Quantum=%d) ===\n", quantum);
    printf("Scheduling: Priority-based with Round Robin (quantum=%d) within same priority\n\n", quantum);

    while (finished_processes < total_processes) {


        for (int i = 0; i < total_processes; i++) {
            if (pcbs[i].in_io && !pcbs[i].finished) {
                pcbs[i].io_remaining--;
                
                if (pcbs[i].io_remaining <= 0) {
                    pcbs[i].in_io = 0;
                    printf("Time %d: %s completes I/O and returns to ready queue (Priority %d)\n", 
                           time, pcbs[i].process.ID, pcbs[i].process.priority);
                    log_print("Time %d: %s completes I/O\n", time, pcbs[i].process.ID);
                }
            }
        }

        PCB* next = NULL;
        int highest_priority = -1;
        

        for (int i = 0; i < total_processes; i++) {
            if (!pcbs[i].finished &&
                !pcbs[i].in_io &&
                pcbs[i].process.arrival_time <= time) {
                
                if (pcbs[i].process.priority > highest_priority) {
                    highest_priority = pcbs[i].process.priority;
                }
            }
        }
        

        if (last_executed != NULL &&
            !last_executed->finished &&
            !last_executed->in_io &&
            last_executed->process.arrival_time <= time &&
            last_executed->process.priority == highest_priority &&
            current_quantum_used < quantum) {
            next = last_executed;
        } else {

            current_quantum_used = 0;
            
            for (int i = 0; i < total_processes; i++) {
                if (!pcbs[i].finished &&
                    !pcbs[i].in_io &&
                    pcbs[i].process.arrival_time <= time &&
                    pcbs[i].process.priority == highest_priority) {
                    
                    if (!next) {
                        next = &pcbs[i];
                    } else if (pcbs[i].process.arrival_time < next->process.arrival_time) {

                        next = &pcbs[i];
                    }
                }
            }
        }

        if (next) {
            printf("Time %d: |%-4s (Priority %d, Quantum: %d/%d) | Progress: %d/%d\n",
                   time,
                   next->process.ID,
                   next->process.priority,
                   current_quantum_used + 1,
                   quantum,
                   next->executed_time + 1,
                   next->process.execution_time);
            
            log_print("Time %d: |%-4s\n", time, next->process.ID);
        
            add_gantt_slice(next->process.ID, time, 1, NULL);

            next->remaining_time--;
            next->executed_time++;
            current_quantum_used++;
            last_executed = next;


            if (next->io_index < next->process.io_count && next->remaining_time > 0) {
                IO_OPERATION *io_op = &next->process.io_operations[next->io_index];
                
                if (next->executed_time >= io_op->start_time) {
                    next->io_index++;

                    next->io_remaining = io_op->duration + 1;
                    next->in_io = 1;

                    add_io_slice(next->process.ID, time + 1, io_op->duration, NULL, "I/O");
                    
                    printf("  -> %s blocks for I/O at time %d for %d units (will complete at time %d)\n", 
                           next->process.ID, time + 1, io_op->duration, time + 1 + io_op->duration);
                    log_print("  -> %s enters I/O for %d units\n", 
                             next->process.ID, io_op->duration);

                    current_quantum_used = 0;
                    last_executed = NULL;
                }
            }

            if (current_quantum_used >= quantum && next->remaining_time > 0 && !next->in_io) {
                printf("  -> %s quantum expired, moving to back of queue\n", next->process.ID);
                current_quantum_used = 0;
                last_executed = NULL;
            }

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished_processes++;
                printf("Process %s finished at time %d\n",
                       next->process.ID, time + 1);
                log_print("Process %s finished at time %d\n",
                       next->process.ID, time + 1);
                
                current_quantum_used = 0;
                last_executed = NULL;
            }
        } else {

            printf("Time %d: CPU idle\n", time);
            log_print("Time %d: CPU idle\n", time);
            add_gantt_slice("IDLE", time, 1, "#cccccc");
            
            current_quantum_used = 0;
            last_executed = NULL;
        }
        
        time++;
    }

    printf("\n=== Multilevel Static Scheduler End (Total time: %d) ===\n", time);
}