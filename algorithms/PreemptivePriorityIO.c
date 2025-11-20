#include <stdio.h>
#include <stdlib.h>
#include "../config.h"

// PCB structure for scheduling

typedef struct {
    PROCESS *p;
    int remaining_time;
    int executed_time;
    int next_io;
    int io_remaining;
    int finished;
    int in_io;
} PCB;

int all_finished(PCB pcb[], int n) {
    for (int i = 0; i < n; i++)
        if (!pcb[i].finished) return 0;
    return 1;
}

// find highest priority ready process
int select_process(PCB pcb[], int current, int n, int time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (pcb[i].finished) continue;
        if (pcb[i].in_io) continue;
        if (pcb[i].p->arrival_time > time) continue;

        if (best == -1 || pcb[i].p->priority < pcb[best].p->priority) {
            best = i;
        }
    }
    return best;
}

void run_priority_preemptive(PROCESS p[], int count) {
    PCB pcb[count];

    for (int i = 0; i < count; i++) {
        pcb[i].p = &p[i];
        pcb[i].remaining_time = p[i].execution_time;
        pcb[i].executed_time = 0;
        pcb[i].next_io = 0;
        pcb[i].io_remaining = 0;
        pcb[i].finished = 0;
        pcb[i].in_io = 0;
    }

    int time = 0;
    int running = -1;

    printf("--- Simulation Start ---\n");

    while (!all_finished(pcb, count)) {

        // handle IO
        for (int i = 0; i < count; i++) {
            if (pcb[i].in_io) {
                pcb[i].io_remaining--;
                if (pcb[i].io_remaining == 0) {
                    pcb[i].in_io = 0;
                    printf("t=%d: P%d finished IO and returned to ready queue\n", time, pcb[i].p->id);
                }
            }
        }

        // check if running process must enter IO
        if (running != -1) {
            int idx = running;
            if (pcb[idx].next_io < pcb[idx].p->io_count) {
                IO_OPERATION *op = &pcb[idx].p->io_operations[pcb[idx].next_io];
                if (pcb[idx].executed_time == op->start_time) {
                    printf("t=%d: P%d enters IO (%d units)\n", time, pcb[idx].p->id, op->duration);
                    pcb[idx].in_io = 1;
                    pcb[idx].io_remaining = op->duration;
                    pcb[idx].next_io++;
                    running = -1;
                }
            }
        }

        // select next process
        int best = select_process(pcb, running, count, time);

        // preempt if needed
        if (best != -1 && best != running) {
            if (running != -1)
                printf("t=%d: P%d preempted by P%d\n", time, pcb[running].p->id, pcb[best].p->id);
            running = best;
            printf("t=%d: P%d is running\n", time, pcb[running].p->id);
        }

        // execute 1 time unit
        if (running != -1) {
            pcb[running].remaining_time--;
            pcb[running].executed_time++;

            if (pcb[running].remaining_time == 0) {
                printf("t=%d: P%d finished execution\n", time + 1, pcb[running].p->id);
                pcb[running].finished = 1;
                running = -1;
            }
        }

        time++;
    }

    printf("--- Simulation End at t=%d ---\n", time);
}
