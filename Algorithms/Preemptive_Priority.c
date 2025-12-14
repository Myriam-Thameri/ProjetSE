/*
 * Simulateur d'Ordonnancement de Processus - Priority Preemptive
 * Adapted to use linked list QUEUE data structure
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Config/config.h"
#include "../Config/types.h"
#include "../Utils/Algorithms.h"
#include "../Interface/gantt_chart.h"
#include "../Utils/log_file.h"

static int is_queue_empty(QUEUE q) {
    return q.size == 0;
}

/* Pick highest priority process from queue and remove it */
static PCB* pick_highest_priority_and_remove(QUEUE *q, PCB *pcb, int count, int time) {
    if (is_queue_empty(*q)) return NULL;
    
    QueueNode *node = q->start;
    PCB *best_pcb = NULL;
    
    // Find the highest priority process
    while (node != NULL) {
        PCB *current_pcb = find_pcb_by_id(pcb, count, node->process.ID);
        
        if (current_pcb && !current_pcb->in_io && !current_pcb->finished &&
            current_pcb->process.arrival_time <= time) {
            
            if (best_pcb == NULL ||
                current_pcb->process.priority < best_pcb->process.priority ||
                (current_pcb->process.priority == best_pcb->process.priority &&
                 current_pcb->process.arrival_time < best_pcb->process.arrival_time)) {
                best_pcb = current_pcb;
            }
        }
        node = node->next;
    }
    
    if (best_pcb) {
        // Remove this process from queue
        *q = remove_specific_process(*q, best_pcb->process.ID);
    }
    
    return best_pcb;
}

/* Check if a process needs IO after current execution */
static int needs_io_after_current_execution(PCB *p) {
    if (p->io_index >= p->process.io_count) return 0;
    
    IO_OPERATION *io_op = &p->process.io_operations[p->io_index];
    
    if (p->executed_time + 1 == io_op->start_time) {
        return 1;
    }
    
    return 0;
}

/* Process IO queue - handle all processes in IO */
static void process_io_queue(QUEUE *ioq, QUEUE *readyq, PCB *pcb, int count, int time) {
    if (is_queue_empty(*ioq)) return;

    QueueNode *node = ioq->start;
    
    while (node != NULL) {
        PCB *p = find_pcb_by_id(pcb, count, node->process.ID);
        QueueNode *next_node = node->next;
        
        if (p && p->in_io) {
            p->io_remaining--;
            
            if (p->io_remaining <= 0) {
                // Remove from IO queue and add to ready queue
                *ioq = remove_specific_process(*ioq, p->process.ID);
                p->in_io = 0;
                p->io_remaining = 0;
                *readyq = add_process_to_queue(*readyq, p->process);
                
                printf("t=%d: %s completes IO and becomes READY\n", time, p->process.ID);
                log_print("t=%d: %s completes IO and becomes READY\n", time, p->process.ID);
            }
        }
        
        node = next_node;
    }
}

/* Update wait times for processes in ready queue */
static void update_wait_times(QUEUE *readyq, PCB *pcb, int count, PCB *running, int time) {
    QueueNode *node = readyq->start;
    
    while (node != NULL) {
        PCB *p = find_pcb_by_id(pcb, count, node->process.ID);
        
        if (p && p->process.arrival_time <= time && p != running && !p->finished) {
            p->wait_time++;
        }
        node = node->next;
    }
}

/* ------------------- MAIN SIMULATION ------------------- */
void run_priority_preemptive(Config *config) {
    if (!config || config->process_count <= 0) return;
    
    clear_gantt_slices();
    clear_io_slices();
    
    int count = config->process_count;
    PCB *pcbs = initialize_PCB(config);
    
    QUEUE readyq = {NULL, NULL, 0};
    QUEUE ioq = {NULL, NULL, 0};

    /* Add all processes to ready queue */
    for (int i = 0; i < count; i++) {
        readyq = add_process_to_queue(readyq, pcbs[i].process);
    }

    PCB *running = NULL;
    int time = 0;

    printf("--- Simulation Start ---\n");
    log_print("--- Priority Preemptive Algorithm Started ***\n\n");

    while (1) {
        printf("\nt=%d: ", time);

        /* STEP 1 — Process IO queue first */
        process_io_queue(&ioq, &readyq, pcbs, count, time);

        /* STEP 2 — Schedule if CPU is free */
        if (!running) {
            PCB *next = pick_highest_priority_and_remove(&readyq, pcbs, count, time);
            if (next) {
                running = next;
                printf("%s starts running\n", running->process.ID);
                log_print("%s starts running\n", running->process.ID);
            }
        }
        
        /* STEP 3 — Check preemption */
        if (running) {
            PCB *higher = pick_highest_priority_and_remove(&readyq, pcbs, count, time);
            if (higher) {
                if (higher->process.priority < running->process.priority) {
                    printf("%s preempted by %s\n", running->process.ID, higher->process.ID);
                    log_print("%s preempted by %s\n", running->process.ID, higher->process.ID);
                    readyq = add_process_to_queue(readyq, running->process);
                    running = higher;
                } else {
                    readyq = add_process_to_queue(readyq, higher->process);
                }
            }
        }

        /* STEP 4 — Execute or handle IO */
        if (running) {
            printf("%s executes\n", running->process.ID);
            log_print("%s executes\n", running->process.ID);
            add_gantt_slice(running->process.ID, time, 1, NULL);
            running->remaining_time--;
            running->executed_time++;
            
            // Check if process finished
            if (running->remaining_time <= 0) {
                printf("t=%d: %s FINISHED\n", time + 1, running->process.ID);
                log_print("t=%d: %s FINISHED\n", time + 1, running->process.ID);
                running->finished = 1;
                running = NULL;
            } 
            // Check for I/O after execution
            else if (needs_io_after_current_execution(running)) {
                IO_OPERATION *io_op = &running->process.io_operations[running->io_index];
                running->io_index++;
                
                // CRITICAL FIX: Set io_remaining to duration + 1
                // Because it will be decremented at the start of NEXT iteration
                running->io_remaining = io_op->duration + 1;
                running->in_io = 1;
                ioq = add_process_to_queue(ioq, running->process);
                
                // Add I/O slice starting at next time unit
                add_io_slice(running->process.ID, time + 1, io_op->duration, NULL, "I/O");
                
                printf("t=%d: %s enters IO for %d units (will complete at t=%d)\n", 
                       time + 1, running->process.ID, io_op->duration, time + 1 + io_op->duration);
                log_print("t=%d: %s enters IO for %d units\n", 
                          time + 1, running->process.ID, io_op->duration);
                running = NULL;
            }
        } else {
            int io_busy = !is_queue_empty(ioq);
            if (io_busy) {
                printf("CPU idle (IO device busy)\n");
                log_print("CPU idle (IO device busy)\n");
            } else if (!is_queue_empty(readyq)) {
                printf("CPU idle (processes in ready queue)\n");
                log_print("CPU idle (processes in ready queue)\n");
            } else {
                printf("CPU idle\n");
                log_print("CPU idle\n");
            }
            add_gantt_slice("IDLE", time, 1, "#cccccc");
        }

        /* STEP 5 — Update wait times */
        update_wait_times(&readyq, pcbs, count, running, time);

        /* STEP 6 — Check for completion */
        int all_done = 1;
        for (int i = 0; i < count; i++) {
            if (!pcbs[i].finished) {
                all_done = 0;
                break;
            }
        }
        
        if (all_done && !running && is_queue_empty(readyq) && is_queue_empty(ioq)) {
            break;
        }

        time++;
    }

    printf("\n--- Simulation End at t=%d ---\n", time);
    log_print("\n--- Priority Preemptive Algorithm Completed ***\n\n");
    
    // Print summary
    printf("\nProcess Summary:\n");
    printf("Process\tWait Time\n");
    for (int i = 0; i < count; i++) {
        printf("%s\t%d\n", pcbs[i].process.ID, pcbs[i].wait_time);
    }
    
    // Clean up queues
    while (!is_queue_empty(readyq)) {
        readyq = remove_process_from_queue(readyq);
    }
    while (!is_queue_empty(ioq)) {
        ioq = remove_process_from_queue(ioq);
    }
}