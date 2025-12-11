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
#include "../Config/config.h"
#include "../Config/types.h"
#include "../Utils/Algorithms.h"
#include "../Interface/gantt_chart.h"

#define MAX_QUEUE 256

/* ------------------- QUEUE ------------------- */
typedef struct {
    PCB* arr[MAX_QUEUE];
    int front;
    int rear;
} Queue;

static void q_init(Queue *q) { q->front = q->rear = 0; }
static int q_empty(Queue *q) { return q->front == q->rear; }
static void q_enqueue(Queue *q, PCB *x) { 
    if (q->rear < MAX_QUEUE) {
        q->arr[q->rear++] = x;
    }
}
static PCB* q_dequeue(Queue *q) { 
    return q_empty(q) ? NULL : q->arr[q->front++];
}

/* pick highest priority among ready items (remove from queue) */
static PCB* pick_highest_priority_and_remove(Queue *q, int time) {
    int idx = -1;

    for (int i = q->front; i < q->rear; ++i) {
        PCB *c = q->arr[i];
        
        if (c->in_io) continue; 
        if (c->process.arrival_time <= time && !c->finished) {
            if (idx == -1 ||
                c->process.priority < q->arr[idx]->process.priority ||
                (c->process.priority == q->arr[idx]->process.priority &&
                 c->process.arrival_time < q->arr[idx]->process.arrival_time)) {
                idx = i;
            }
        }
    }

    if (idx == -1) return NULL;

    PCB *res = q->arr[idx];
    for (int j = idx; j < q->rear - 1; ++j)
        q->arr[j] = q->arr[j + 1];
    q->rear--;

    return res;
}

/* Check if a process needs IO at the END of current time unit */
static int needs_io_after_current_execution(PCB *p) {
    if (p->io_index >= p->process.io_count) return 0;
    
    IO_OPERATION *io_op = &p->process.io_operations[p->io_index];
    
    // Check if after this execution, the executed_time will reach IO start time
    if (p->executed_time + 1 == io_op->start_time) {
        return 1;
    }
    
    return 0;
}

/* Process IO queue */
static void process_io_queue(Queue *ioq, Queue *readyq, int time) {
    if (q_empty(ioq)) return;

    PCB *p = ioq->arr[ioq->front];
    p->io_remaining--;

    if (p->io_remaining <= 0) {
        q_dequeue(ioq);
        p->in_io = 0;
        p->io_remaining = 0;
        q_enqueue(readyq, p);
        printf("t=%d: %s completes IO and becomes READY\n", time, p->process.ID);
    }
}

/* ------------------- MAIN SIMULATION ------------------- */
void run_priority_preemptive(Config *config) {
    if (!config || config->process_count <= 0) return;
    clear_gantt_slices();
    int count = config->process_count;
    PCB pcbs[MAX_QUEUE]; 
    Queue readyq, ioq;

    q_init(&readyq);
    q_init(&ioq);

    /* Initialize PCBs */
    for (int i = 0; i < count; i++) {
        pcbs[i].process = config->processes[i];
        pcbs[i].remaining_time = pcbs[i].process.execution_time;
        pcbs[i].executed_time = 0;
        pcbs[i].io_index = 0;
        pcbs[i].io_remaining = 0;
        pcbs[i].finished = 0;
        pcbs[i].in_io = 0;
        pcbs[i].wait_time = 0;
        q_enqueue(&readyq, &pcbs[i]);
    }

    PCB *running = NULL;
    int time = 0;
    int io_device_busy = 0;

    printf("--- Simulation Start ---\n");

    while (1) {
        printf("\nt=%d: ", time);

        /* STEP 1 — Process IO queue first */
        process_io_queue(&ioq, &readyq, time);
        io_device_busy = !q_empty(&ioq);

        /* STEP 2 — Schedule if CPU is free */
        if (!running) {
            PCB *next = pick_highest_priority_and_remove(&readyq, time);
            if (next) {
                running = next;
                printf("%s starts running\n", running->process.ID);
            }
        }
        
        /* STEP 3 — Check preemption (if we have a running process) */
        if (running) {
            PCB *higher = pick_highest_priority_and_remove(&readyq, time);
            if (higher) {
                if (higher->process.priority < running->process.priority) {
                    printf("%s preempted by %s\n", running->process.ID, higher->process.ID);
                    q_enqueue(&readyq, running);
                    running = higher;
                } else {
                    q_enqueue(&readyq, higher);
                }
            }
        }

        /* STEP 4 — Execute or handle IO */
        if (running) {
            // Check if this execution will trigger IO
            if (needs_io_after_current_execution(running)) {
                // Will need IO after this execution
                if (io_device_busy) {
                    // IO device busy - cannot execute
                    printf("%s would trigger IO but device BUSY - skips execution\n", 
                           running->process.ID);
                    q_enqueue(&readyq, running);
                    running = NULL;
                    // CPU is idle this time unit - add idle slice
                    add_gantt_slice("IDLE", time, 1, "#cccccc");
                } else {
                    // Execute then enter IO
                    printf("%s executes (will enter IO after)\n", running->process.ID);
                    add_gantt_slice(running->process.ID, time, 1, NULL);
                    running->remaining_time--;
                    running->executed_time++;
                    
                    if (running->remaining_time <= 0) {
                        printf("t=%d: %s FINISHED\n", time + 1, running->process.ID);
                        running->finished = 1;
                        running = NULL;
                    } else {
                        // Enter IO
                        IO_OPERATION *io_op = &running->process.io_operations[running->io_index];
                        running->io_index++;
                        running->io_remaining = io_op->duration;
                        running->in_io = 1;
                        q_enqueue(&ioq, running);
                        io_device_busy = 1;
                        printf("t=%d: %s enters IO for %d units\n", 
                               time + 1, running->process.ID, io_op->duration);
                        running = NULL;
                    }
                }
            } else {
                // Normal execution - ADD GANTT SLICE HERE
                printf("%s executes\n", running->process.ID);
                add_gantt_slice(running->process.ID, time, 1, NULL);
                running->remaining_time--;
                running->executed_time++;
                
                if (running->remaining_time <= 0) {
                    printf("t=%d: %s FINISHED\n", time + 1, running->process.ID);
                    running->finished = 1;
                    running = NULL;
                }
            }
        } else {
            // CPU idle - ADD IDLE SLICE
            if (io_device_busy) {
                printf("CPU idle (IO device busy)\n");
            } else if (!q_empty(&readyq)) {
                printf("CPU idle (processes in ready queue)\n");
            } else {
                printf("CPU idle\n");
            }
            add_gantt_slice("IDLE", time, 1, "#cccccc");
        }

        /* STEP 5 — Update wait times */
        for (int i = readyq.front; i < readyq.rear; ++i) {
            PCB *p = readyq.arr[i];
            if (p->process.arrival_time <= time && p != running && !p->finished) {
                p->wait_time++;
            }
        }

        /* STEP 6 — Check for completion */
        int all_done = 1;
        for (int i = 0; i < count; i++) {
            if (!pcbs[i].finished) {
                all_done = 0;
                break;
            }
        }
        
        if (all_done && !running && q_empty(&readyq) && q_empty(&ioq)) {
            break;
        }

        time++;
    }

    printf("\n--- Simulation End at t=%d ---\n", time);
    
    // Print summary
    printf("\nProcess Summary:\n");
    printf("Process\tWait Time\n");
    for (int i = 0; i < count; i++) {
        printf("%s\t%d\n", pcbs[i].process.ID, pcbs[i].wait_time);
    }
}