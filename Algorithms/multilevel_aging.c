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

#define QUANTUM 2
#define AGING_INTERVAL 5
#define MAX_PRIORITY 5

void apply_aging(PCB* pcbs, int total, int current_time, PCB* running)
{
    for (int i = 0; i < total; i++) {
        PCB* p = &pcbs[i];

        if (p == running) continue;             
        if (p->finished) continue;              
        if (p->in_io) continue;             
        if (p->process.arrival_time > current_time) continue;

        p->wait_time++;

        if (p->wait_time >= AGING_INTERVAL) {
            if (p->process.priority < MAX_PRIORITY)
                p->process.priority++;

            p->wait_time = 0;

            printf("Aging: Process %s priority increased to %d\n",
                p->process.ID, p->process.priority);
        }
    }
}

void MultilevelAgingScheduler(Config* config)
{
    PCB* pcbs = initialize_PCB(config);
    int total = config->process_count;
    int finished = 0;
    int time = 0;

    for(int i = 0; i < total; i++)
        pcbs[i].wait_time = 0;

    printf("=== Multilevel Scheduler With Aging (Dynamic Priority) ===\n");

    while (finished < total)
    {
        PCB* next = NULL;

        for (int i = 0; i < total; i++) {
            PCB* p = &pcbs[i];

            if (!p->finished && !p->in_io && p->process.arrival_time <= time) {
                if (!next || p->process.priority > next->process.priority) {
                    next = p;
                }
            }
        }
        apply_aging(pcbs, total, time, next);
        if (next) {
            int run_for = QUANTUM;
            if (next->remaining_time < run_for)
                run_for = next->remaining_time;

            printf("Time %d-%d: Running %s (priority=%d)\n",
                time, time + run_for, next->process.ID, next->process.priority);

            next->remaining_time -= run_for;
            next->wait_time = 0;
            time += run_for;

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished++;
                printf("Process %s finished.\n", next->process.ID);
            }

        } else {
            printf("Time %d: CPU idle\n", time);
            time++;
        }
    }

    printf("=== Scheduler End ===\n");
}
