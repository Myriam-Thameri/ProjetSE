/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "../Config/types.h"
#include "../Config/config.h"
#include "../Utils/Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Interface/gantt_chart.h"



/* =================== FCFS ALGORITHM =================== */
void FCFS_Algo(Config* config) {
    clear_gantt_slices();
    
    PCB* pcb = initialize_PCB(config);

    QUEUE ready = { NULL, NULL, 0 };
    QUEUE ioq   = { NULL, NULL, 0 };

    int time = 0;
    int finished = 0;

    // Track current executing process for Gantt chart
    char current_executing[32] = "";
    int slice_start = 0;

    while (finished < config->process_count) {

        /* ----------------- 1. NOUVEAUX ARRIVANTS ----------------- */
        for (int i = 0; i < config->process_count; i++) {
            PROCESS p = config->processes[i];
            if (p.arrival_time == time && !pcb[i].finished && !pcb[i].in_io) {
                ready = add_process_to_queue(ready, p);
                printf("[t=%d] Arrival: %s → ready queue\n", time, p.ID);
            }
        }

        /* ----------------- 2. GESTION DES I/O TERMINÉS ----------------- */
        PROCESS io_finished_list[50];
        int io_finished_count = 0;

        QueueNode* node = ioq.start;
        QueueNode* prev = NULL;

        while (node) {
            PCB* current_pcb = find_pcb_by_id(pcb, config->process_count, node->process.ID);
            QueueNode* next_node = node->next;

            if (current_pcb) {
                current_pcb->io_remaining--;

                if (current_pcb->io_remaining <= 0) {
                    printf("[t=%d] %s: I/O finished → ready queue\n", time, current_pcb->process.ID);
                    current_pcb->in_io = 0;
                    current_pcb->io_index++;

                    io_finished_list[io_finished_count++] = current_pcb->process;

                    if (prev) prev->next = next_node;
                    else ioq.start = next_node;
                    if (node == ioq.end) ioq.end = prev;
                    free(node);
                    ioq.size--;
                    node = next_node;
                    continue;
                }
            }

            prev = node;
            node = next_node;
        }

        for (int i = 0; i < io_finished_count; i++) {
            ready = add_process_to_queue(ready, io_finished_list[i]);
        }

        /* ----------------- 3. EXÉCUTION CPU ----------------- */
        char executing_now[32] = "";
        
        if (ready.size > 0) {
            QueueNode* front = ready.start;
            PROCESS p = front->process;
            PCB* current_pcb = find_pcb_by_id(pcb, config->process_count, p.ID);

            if (current_pcb) {
                // Check if process needs to start I/O
                int start_io = 0;
                if (p.io_count > 0 &&
                    current_pcb->io_index < p.io_count &&
                    current_pcb->executed_time == p.io_operations[current_pcb->io_index].start_time) {
                    start_io = 1;
                }

                if (start_io) {
                    // Move process to I/O queue
                    ready = remove_process_from_queue(ready);
                    current_pcb->in_io = 1;
                    // set io_remaining to duration+1 so the subsequent decrement logic
                    // correctly handles zero-duration I/O operations
                    current_pcb->io_remaining = p.io_operations[current_pcb->io_index].duration + 1;
                    
                    ioq = add_process_to_queue(ioq, p);
                    printf("[t=%d] %s → starts I/O (duration=%d)\n", time, p.ID,
                           p.io_operations[current_pcb->io_index].duration);
                    add_io_slice(p.ID, time, p.io_operations[current_pcb->io_index].duration, NULL, "I/O");
                    // Check if there's another process ready to execute
                    front = ready.start;
                    if (front) {
                        p = front->process;
                        current_pcb = find_pcb_by_id(pcb, config->process_count, p.ID);
                    } else {
                        current_pcb = NULL;
                    }
                }

                // Execute current process if available
                if (current_pcb) {
                    strcpy(executing_now, current_pcb->process.ID);
                    current_pcb->executed_time++;
                    current_pcb->remaining_time--;
                    printf("[t=%d] CPU → %s (executed=%d, remaining=%d)\n", 
                           time, current_pcb->process.ID, 
                           current_pcb->executed_time, current_pcb->remaining_time);

                    // Check if process is finished
                    if (current_pcb->remaining_time <= 0) {
                        ready = remove_process_from_queue(ready);
                        current_pcb->finished = 1;
                        finished++;
                        printf("[t=%d] %s → FINISHED\n", time, current_pcb->process.ID);
                    }
                }
            }
        }
        
        /* ----------------- 4. UPDATE GANTT CHART SLICES ----------------- */
        if (strcmp(current_executing, executing_now) != 0) {
            // Process switched, save the previous slice
            if (strlen(current_executing) > 0) {
                int duration = time - slice_start;
                add_gantt_slice(current_executing, slice_start, duration, NULL);
            }
            
            // Start tracking new process
            strcpy(current_executing, executing_now);
            slice_start = time;
        }

        time++;
    }
    
    /* ----------------- Save final Gantt slice ----------------- */
    if (strlen(current_executing) > 0) {
        int duration = time - slice_start;
        add_gantt_slice(current_executing, slice_start, duration, NULL);
    }

    /* ----------------- AFFICHAGE FINAL ----------------- */
    printf("\n");
    printf("==================== GANTT CHART ====================\n");
    printf("Temps total: %d\n", time);
    printf("=====================================================\n");

    // Clean up queues
    while (ready.start) {
        QueueNode* temp = ready.start;
        ready.start = ready.start->next;
        free(temp);
    }
    
    while (ioq.start) {
        QueueNode* temp = ioq.start;
        ioq.start = ioq.start->next;
        free(temp);
    }

    // DON'T free(pcb) - it's a static array!
}
