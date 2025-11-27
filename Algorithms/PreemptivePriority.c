#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Config/config.h"
#include "../Config/types.h"
#include "Algorithms.h"

#define MAX_QUEUE 256

typedef struct PCB {
    PROCESS *p;
    int remaining_time;
    int executed_time;
    int next_io;
    int io_remaining;
    int finished;
    int arrived;
} PCB;

/* ------------------- QUEUE ------------------- */
typedef struct {
    PCB* arr[MAX_QUEUE];
    int front;
    int rear;
} Queue;

static void q_init(Queue *q) { q->front = q->rear = 0; }
static int q_empty(Queue *q) { return q->front == q->rear; }
static void q_enqueue(Queue *q, PCB *x) { if (q->rear < MAX_QUEUE) q->arr[q->rear++] = x; }
static PCB* q_dequeue(Queue *q) { return q_empty(q) ? NULL : q->arr[q->front++]; }
static int q_size(Queue *q) { return q->rear - q->front; }

/* pick highest priority among ready items (remove from queue) */
static PCB* pick_highest_priority_and_remove(Queue *q, int time) {
    int idx = -1;

    for (int i = q->front; i < q->rear; ++i) {
        PCB *c = q->arr[i];
        if (c->finished) continue;

        if (!c->arrived && c->p->arrival_time <= time)
            c->arrived = 1;

        if (!c->arrived) continue;

        if (idx == -1 ||
            c->p->priority < q->arr[idx]->p->priority ||
            (c->p->priority == q->arr[idx]->p->priority &&
             c->p->arrival_time < q->arr[idx]->p->arrival_time)) {
            idx = i;
        }
    }

    if (idx == -1) return NULL;

    PCB *res = q->arr[idx];
    for (int j = idx; j < q->rear - 1; ++j)
        q->arr[j] = q->arr[j + 1];
    q->rear--;

    return res;
}

/* Process IO queue BEFORE any scheduling */
static void process_io_queue(Queue *ioq, Queue *readyq, int time) {
    PCB *tmp[MAX_QUEUE];
    int count = 0;

    while (!q_empty(ioq)) {
        PCB *x = q_dequeue(ioq);

        x->io_remaining--;

        if (x->io_remaining <= 0) {
            x->arrived = 1;
            q_enqueue(readyq, x);
            printf("t=%d: %s completes IO and becomes READY\n", time, x->p->ID);
        } else {
            tmp[count++] = x;
        }
    }

    for (int i = 0; i < count; i++)
        q_enqueue(ioq, tmp[i]);
}

/* ------------------- MAIN SIMULATION ------------------- */
void run_priority_preemptive(PROCESS p[], int count) {
    if (count <= 0) return;

    PCB pcb[count];
    Queue readyq, ioq;

    q_init(&readyq);
    q_init(&ioq);

    /* Initialize PCB */
    for (int i = 0; i < count; i++) {
        pcb[i].p = &p[i];
        pcb[i].remaining_time = p[i].execution_time;
        pcb[i].executed_time = 0;
        pcb[i].next_io = 0;
        pcb[i].io_remaining = 0;
        pcb[i].finished = 0;
        pcb[i].arrived = 0;
        q_enqueue(&readyq, &pcb[i]);
    }

    PCB *running = NULL;
    int time = 0;

    printf("--- Simulation Start ---\n");

    while (1) {

        /* STEP 1 — Process IO FIRST */
        process_io_queue(&ioq, &readyq, time);

        /* STEP 2 — If CPU is running, first check IO triggers AFTER executed_time update */
        if (running) {
            /* NOT checking IO here */
        }

        /* STEP 3 — Select next process by priority */
        PCB *next = pick_highest_priority_and_remove(&readyq, time);

        if (next) {
            if (running) {
                if (next->p->priority < running->p->priority) {
                    printf("t=%d: %s preempted by %s\n",
                           time, running->p->ID, next->p->ID);
                    q_enqueue(&readyq, running);
                    running = next;
                    printf("t=%d: %s starts running\n", time, running->p->ID);
                } else {
                    q_enqueue(&readyq, next);
                }
            } else {
                running = next;
                printf("t=%d: %s starts running\n", time, running->p->ID);
            }
        }

        /* STEP 4 — Execute one time unit */
        if (running) {
            running->remaining_time--;
            running->executed_time++;

            /* STEP 5 — NOW check IO trigger AFTER executed_time++ */
            if (running->next_io < running->p->io_count) {
                IO_OPERATION *op = &running->p->io_operations[running->next_io];

                if (running->executed_time == op->start_time) {
                    running->next_io++;

                    if (op->duration > 0) {
                        running->io_remaining = op->duration;
                        q_enqueue(&ioq, running);
                        printf("t=%d: %s enters IO (%d units)\n",
                               time+1, running->p->ID, op->duration);
                        running = NULL;
                    }
                }
            }

            /* STEP 6 — Only check finish AFTER IO check */
            if (running && running->remaining_time <= 0) {
                printf("t=%d: %s FINISHED\n", time + 1, running->p->ID);
                running->finished = 1;
                running = NULL;
            }

        } else {
            printf("t=%d: CPU idle\n", time);
        }

        /* Check global finish */
        int done = 1;
        for (int i = 0; i < count; i++)
            if (!pcb[i].finished) done = 0;

        if (done) break;

        time++;
    }

    printf("--- Simulation End at t=%d ---\n", time);
}
