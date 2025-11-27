#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================== Initialisation PCB =================== */
PCB* initialize_PCB(Config* config) {
    PCB* pcb = malloc(sizeof(PCB) * config->process_count);
    if (!pcb) {
        printf("Erreur d'allocation mémoire !\n");
        exit(1);
    }

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

/* =================== Queue utils =================== */
QUEUE add_process_to_queue(QUEUE q, PROCESS p) {
    QueueNode* node = malloc(sizeof(QueueNode));
    if (!node) {
        printf("Erreur d'allocation mémoire !\n");
        exit(1);
    }
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
    if (q.size == 0) q.end = NULL;
    return q;
}

/* =================== Trouver PCB par ID =================== */
PCB* find_pcb_by_id(PCB* pcb, int count, const char* id) {
    for (int i = 0; i < count; i++) {
        if (strcmp(pcb[i].process.ID, id) == 0) {
            return &pcb[i];
        }
    }
    return NULL;
}

/* =================== FCFS ALGORITHM =================== */
void FCFS_Algo(Config* config) {

    PCB* pcb = initialize_PCB(config);

    QUEUE ready = { NULL, NULL, 0 };
    QUEUE ioq   = { NULL, NULL, 0 };

    int time = 0;
    int finished = 0;

    char gantt[4096] = "";
    char gantt_t[4096] = "0";

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

                if (current_pcb->io_remaining == 0) {
                    printf("[t=%d] %s: I/O finished → ready queue\n", time, current_pcb->process.ID);
                    current_pcb->in_io = 0;
                    current_pcb->io_index++;

                    io_finished_list[io_finished_count++] = current_pcb->process;

                    if (prev) prev->next = next_node;
                    else ioq.start = next_node;
                    if (node == ioq.end) ioq.end = prev;
                    free(node);
                    ioq.size--;
                }
            }

            prev = node;
            node = next_node;
        }

        for (int i = 0; i < io_finished_count; i++) {
            ready = add_process_to_queue(ready, io_finished_list[i]);
        }

        /* ----------------- 3. EXÉCUTION CPU ----------------- */
        if (ready.size > 0) {
            QueueNode* front = ready.start;
            PROCESS p = front->process;
            PCB* current_pcb = find_pcb_by_id(pcb, config->process_count, p.ID);

            if (current_pcb) {
                int start_io = 0;
                if (p.io_count > 0 &&
                    current_pcb->io_index < p.io_count &&
                    current_pcb->executed_time == p.io_operations[current_pcb->io_index].start_time) {
                    start_io = 1;
                }

                if (start_io) {
                    ready = remove_process_from_queue(ready);
                    current_pcb->in_io = 1;
                    current_pcb->io_remaining = p.io_operations[current_pcb->io_index].duration;
                    ioq = add_process_to_queue(ioq, p);
                    printf("[t=%d] %s → starts I/O (duration=%d)\n", time, p.ID,
                           p.io_operations[current_pcb->io_index].duration);

                    // ne rien exécuter ce tick si CPU vide
                    front = ready.start;
                    if (!front) current_pcb = NULL;
                    else {
                        p = front->process;
                        current_pcb = find_pcb_by_id(pcb, config->process_count, p.ID);
                    }
                }

                if (current_pcb) {
                    current_pcb->executed_time++;
                    current_pcb->remaining_time--;
                    printf("[t=%d] CPU → %s\n", time, current_pcb->process.ID);

                    char block[16];
                    sprintf(block, "|%-4s", current_pcb->process.ID);
                    strcat(gantt, block);

                    if (current_pcb->remaining_time == 0) {
                        ready = remove_process_from_queue(ready);
                        current_pcb->finished = 1;
                        finished++;
                        printf("[t=%d] %s → FINISHED\n", time, current_pcb->process.ID);
                    }
                }
            }
        }

        /* ----------------- Mise à jour de la timeline Gantt ----------------- */
        time++;
        char tb[16];
        sprintf(tb, " %d", time);
        strcat(gantt_t, tb);
    }

    /* ----------------- AFFICHAGE FINAL ----------------- */
    printf("\n");
    printf("==================== GANTT CHART ====================\n");
    printf("%s|\n", gantt);
    printf("%s\n", gantt_t);
    printf("Temps total: %d\n", time);
    printf("=====================================================\n");

    free(pcb);
}