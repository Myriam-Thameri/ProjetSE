#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PCB* initialize_PCB(Config* config) {
    static PCB pcb[50];
    for (int i = 0; i < config->process_count; i++) {
        pcb[i].process = config->processes[i];
        pcb[i].remaining_time = config->processes[i].execution_time;
        pcb[i].executed_time = 0;
        pcb[i].io_index = 0;
        pcb[i].in_io = 0;
        pcb[i].io_remaining = 0;
        pcb[i].finished = 0;
    }
    return pcb;
}

QUEUE add_process_to_queue(QUEUE q, PROCESS p) {
    QueueNode* node = malloc(sizeof(QueueNode));
    node->process = p;
    node->next = NULL;

    if (q.size == 0) {
        q.start = q.end = node;
    } else {
        q.end->next = node;
        q.end = node;
    }
    q.size++;
    return q;
}

QUEUE remove_process_from_queue(QUEUE q) {
    if (q.size == 0) return q;
    QueueNode* tmp = q.start;
    q.start = q.start->next;
    free(tmp);
    q.size--;
    return q;
}

/* ======================================================================= */
/* ============================= FCFS ALGORITHM =========================== */
/* ======================================================================= */

void FCFS_Algo(Config* config) {

    PCB* pcb = initialize_PCB(config);

    QUEUE ready = { NULL, NULL, 0 };
    QUEUE ioq   = { NULL, NULL, 0 };

    int time = 0;
    int finished = 0;

    char gantt[4096] = "";
    char gantt_t[4096] = "0 ";

    while (finished < config->process_count) {

        /* ========== ARRIVALS ========== */
        for (int i = 0; i < config->process_count; i++) {
            PROCESS p = config->processes[i];
            if (p.arrival_time == time && !pcb[i].finished && !pcb[i].in_io) {
                ready = add_process_to_queue(ready, p);
                printf("[t=%d] Arrival: %s → ready queue\n", time, p.ID);
            }
        }

        /* ========== I/O MANAGEMENT ========== */
        if (ioq.size > 0) {
            PROCESS p = ioq.start->process;

            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(pcb[i].process.ID, p.ID) == 0) {
                    pcb[i].io_remaining--;

                    printf("[t=%d] %s: I/O running (%d left)\n",
                           time, p.ID, pcb[i].io_remaining);

                    if (pcb[i].io_remaining == 0) {
                        ioq = remove_process_from_queue(ioq);
                        pcb[i].in_io = 0;
                        pcb[i].io_index++;

                        ready = add_process_to_queue(ready, p);
                        printf("[t=%d] %s: I/O finished → ready queue\n", time, p.ID);
                    }
                }
            }
        }

        /* ========== CPU EXECUTION ========== */
        if (ready.size > 0) {
            PROCESS p = ready.start->process;

            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(pcb[i].process.ID, p.ID) == 0) {

                    /* exécution */
                    pcb[i].executed_time++;
                    pcb[i].remaining_time--;

                    printf("[t=%d] CPU → %s\n", time, p.ID);

                    char block[16];
                    sprintf(block, "|%s", p.ID);
                    strcat(gantt, block);

                    /* I/O trigger */
                    if (p.io_count > 0 &&
                        pcb[i].io_index < p.io_count &&
                        pcb[i].executed_time == p.io_operations[pcb[i].io_index].start_time) {

                        printf("[t=%d] %s → starts I/O\n", time, p.ID);

                        ready = remove_process_from_queue(ready);
                        pcb[i].in_io = 1;
                        pcb[i].io_remaining = p.io_operations[pcb[i].io_index].duration;

                        ioq = add_process_to_queue(ioq, p);
                    }
                    /* FINISHED */
                    else if (pcb[i].remaining_time == 0) {
                        printf("[t=%d] %s → FINISHED\n", time, p.ID);
                        ready = remove_process_from_queue(ready);
                        pcb[i].finished = 1;
                        finished++;
                    }

                    break;
                }
            }
        }
        else {
            strcat(gantt, "|IDLE");
        }

        /* timeline */
        char tb[16];
        sprintf(tb, " %d ", time + 1);
        strcat(gantt_t, tb);

        time++;
    }

    printf("\n=========== GANTT ===========\n");
    printf("%s\n", gantt);
    printf("%s\n", gantt_t);
}
