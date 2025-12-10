#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Config/config.h"
#include "../Config/types.h"
#include "Algorithms.h"

#define MAX_QUEUE 256
#define MAX_LENGTH_ID 9
#define MAX_GANTT_SLOTS 1000 

char gant[MAX_GANTT_SLOTS][MAX_LENGTH_ID]; 
static int gantt_index = 0;

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

static int needs_io_at_this_moment(PCB *p) {
    if (p->io_index >= p->process.io_count) return 0;
    
    IO_OPERATION *io_op = &p->process.io_operations[p->io_index];
    if (p->executed_time == io_op->start_time && p->io_remaining == 0) {
        return 1;
    }
    
    return 0;
}

/* ------------------- MAIN SIMULATION ------------------- */
void run_priority_preemptive(Config *config) {
    if (!config || config->process_count <= 0) return;

    int count = config->process_count;
    PCB pcbs[MAX_QUEUE]; 
    Queue readyq, ioq, blockedq;

    q_init(&readyq);
    q_init(&ioq);
    q_init(&blockedq);
    
    gantt_index = 0;

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

        process_io_queue(&ioq, &readyq, time);
        
        io_device_busy = !q_empty(&ioq);
        
        if (!io_device_busy && !q_empty(&blockedq)) {

            while (!q_empty(&blockedq)) {
                PCB *blocked = q_dequeue(&blockedq);
                blocked->in_io = 0;
                q_enqueue(&readyq, blocked);
                printf("%s unblocked and becomes READY\n", blocked->process.ID);
            }
        }

        if (running && needs_io_at_this_moment(running)) {
            if (io_device_busy) {

                printf("%s needs IO but device BUSY - becomes BLOCKED\n", running->process.ID);
                running->in_io = 1;
                q_enqueue(&blockedq, running);
                running = NULL;
            } else {
                // Enter IO
                IO_OPERATION *io_op = &running->process.io_operations[running->io_index];
                running->io_index++;
                running->io_remaining = io_op->duration;
                running->in_io = 1;
                q_enqueue(&ioq, running);
                io_device_busy = 1;
                printf("%s enters IO for %d units\n", running->process.ID, io_op->duration);
                running = NULL;
            }
        }


        int i = readyq.front;
        while (i < readyq.rear) {
            PCB *p = readyq.arr[i];
            
            if (needs_io_at_this_moment(p) && p->process.arrival_time <= time) {
                if (io_device_busy) {

                    printf("%s needs IO but device BUSY - becomes BLOCKED\n", p->process.ID);
                    

                    for (int j = i; j < readyq.rear - 1; j++) {
                        readyq.arr[j] = readyq.arr[j + 1];
                    }
                    readyq.rear--;
                    

                    p->in_io = 1; 
                    q_enqueue(&blockedq, p);
                    

                } else {

                    IO_OPERATION *io_op = &p->process.io_operations[p->io_index];
                    

                    for (int j = i; j < readyq.rear - 1; j++) {
                        readyq.arr[j] = readyq.arr[j + 1];
                    }
                    readyq.rear--;
                    

                    p->io_index++;
                    p->io_remaining = io_op->duration;
                    p->in_io = 1;
                    q_enqueue(&ioq, p);
                    io_device_busy = 1;
                    printf("%s enters IO for %d units\n", p->process.ID, io_op->duration);
                    

                }
            } else {
                i++; 
            }
        }


        if (!running) {
            PCB *next = pick_highest_priority_and_remove(&readyq, time);
            if (next) {
                running = next;
                printf("%s starts running\n", running->process.ID);
            }
        }
        

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


        if (running) {
            

            if (gantt_index < MAX_GANTT_SLOTS) {

                strncpy(gant[gantt_index], running->process.ID, MAX_LENGTH_ID - 1);
                gant[gantt_index][MAX_LENGTH_ID - 1] = '\0'; 
                gantt_index++;
            }

            
            printf("%s executes (Executed: %d, Remaining: %d)\n", 
                   running->process.ID, running->executed_time + 1, running->remaining_time - 1);
            

            running->remaining_time--;
            running->executed_time++;
            

            if (running->remaining_time <= 0) {
                printf("t=%d: %s FINISHED\n", time + 1, running->process.ID);
                running->finished = 1;
                running = NULL;
            }
        } else {
            

            if (gantt_index < MAX_GANTT_SLOTS) {

                strncpy(gant[gantt_index], "IDLE", MAX_LENGTH_ID - 1);
                gant[gantt_index][MAX_LENGTH_ID - 1] = '\0';
                gantt_index++;
            }
            // ------------------------------------------


            if (!q_empty(&ioq)) {
                printf("CPU idle (process in IO)\n");
            } else if (!q_empty(&blockedq)) {
                printf("CPU idle (processes blocked waiting for IO)\n");
            } else if (!q_empty(&readyq)) {

                printf("CPU idle (processes in ready queue, but none meet criteria)\n");
            } else {
                printf("CPU idle (no more processes)\n");
            }
        }

        for (int i = readyq.front; i < readyq.rear; ++i) {
            PCB *p = readyq.arr[i];
            if (p->process.arrival_time <= time && p != running && !p->finished) {
                p->wait_time++;
            }
        }


        int all_done = 1;
        for (int i = 0; i < count; i++) {
            if (!pcbs[i].finished) {
                all_done = 0;
                break;
            }
        }
        
        if (all_done && !running && q_empty(&readyq) && q_empty(&ioq) && q_empty(&blockedq)) {
            break;
        }

        time++;
    }

    printf("\n--- Simulation End at t=%d ---\n", time);
    

    printf("\nProcess Summary:\n");
    printf("Process\tWait Time\n");
    for (int i = 0; i < count; i++) {
        printf("%s\t%d\n", pcbs[i].process.ID, pcbs[i].wait_time);
    }

    printf("\n--- Final Gantt Chart ---\n");
    

    printf("Time (t):\t");
    for (int i = 0; i < gantt_index; i++) {
        printf("%d\t", i);
    }
    printf("\n");
    

    printf("Process ID:\t");
    for (int i = 0; i < gantt_index; i++) {
        printf("%s\t", gant[i]);
    }
    printf("\n");

}