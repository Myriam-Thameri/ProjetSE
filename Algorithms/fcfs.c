/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



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
