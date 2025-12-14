/*
 * Simulateur d'Ordonnancement de Processus - Multilevel Aging
 * Adapted to use linked list QUEUE data structure
 * Copyright (c) 2025 Équipe ProjetSE - Université de Tunis El Manar
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "../Config/types.h"
#include "../Config/config.h"
#include "../Utils/Algorithms.h"
#include "../Utils/log_file.h"
#include "../Interface/gantt_chart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void apply_aging(PCB* pcbs, int total, int current_time, PCB* running,
                 int aging_interval, int max_priority)
{
    for (int i = 0; i < total; i++) {
        PCB* p = &pcbs[i];

        if (p == running) continue;
        if (p->finished) continue;
        if (p->in_io) continue;
        if (p->process.arrival_time > current_time) continue;

        p->wait_time++;

        if (p->wait_time >= aging_interval) {
            if (p->process.priority < max_priority)
                p->process.priority++;

            p->wait_time = 0;

            printf("Time %d: Aging applied → %s priority is now %d\n",
                   current_time,
                   p->process.ID,
                   p->process.priority);
            log_print("Time %d: Aging applied → %s priority is now %d\n",
                   current_time,
                   p->process.ID,
                   p->process.priority);
        }
    }
}


void MultilevelAgingScheduler(Config* config, int quantum, int aging_interval, int max_priority)
{
    clear_gantt_slices();
    clear_io_slices();
    
    PCB* pcbs = initialize_PCB(config);
    int total = config->process_count;
    int finished = 0;
    int time = 0;

    printf("\n=== MULTILEVEL SCHEDULER WITH AGING + I/O ===\n");
    printf("Quantum=%d | Aging Interval=%d | Max Priority=%d\n\n", quantum, aging_interval, max_priority);
    log_print("\n=== MULTILEVEL SCHEDULER WITH AGING + I/O ===\n");
    log_print("Quantum=%d | Aging Interval=%d | Max Priority=%d\n\n", quantum, aging_interval, max_priority);

    while (finished < total)
    {
        for (int i = 0; i < total; i++) {
            PCB* p = &pcbs[i];

            if (p->in_io) {
                p->io_remaining--;

                if (p->io_remaining <= 0) {
                    p->in_io = 0;
                    p->io_index++;
                    printf("Time %d: %s finished I/O and returned to READY\n", time, p->process.ID);
                    log_print("Time %d: %s finished I/O and returned to READY\n", time, p->process.ID);
                }
            }
        }

        PCB* next = NULL;
        for (int i = 0; i < total; i++) {
            PCB* p = &pcbs[i];

            if (!p->finished && !p->in_io &&
                p->process.arrival_time <= time) {

                if (!next || p->process.priority > next->process.priority) {
                    next = p;
                }
            }
        }

        apply_aging(pcbs, total, time, next, aging_interval, max_priority);

        if (next) {
            int run_for = quantum;
            if (next->remaining_time < run_for)
                run_for = next->remaining_time;

            int actual_run = 0;
            for (int t = 0; t < run_for; t++) {
                add_gantt_slice(next->process.ID, time + t, 1, NULL);

                next->remaining_time--;
                next->executed_time++;
                actual_run++;

                if (next->io_index < next->process.io_count) {
                    IO_OPERATION io = next->process.io_operations[next->io_index];
                    if (next->executed_time == io.start_time) {
                        next->in_io = 1;
                        next->io_remaining = io.duration;

                        add_io_slice(next->process.ID, time + t + 1, io.duration, NULL, "I/O");

                        t++;
                        break;
                    }
                }
            }


            next->wait_time = 0;
            time += actual_run;

            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished++;
                printf("Time %d: %s has COMPLETED execution\n", time, next->process.ID);
                log_print("Time %d: %s has COMPLETED execution\n", time, next->process.ID);
            }

        } else {
            printf("Time %d: CPU is IDLE\n", time);
            log_print("Time %d: CPU is IDLE\n", time);
            add_gantt_slice("IDLE", time, 1, "#cccccc");
            time++;
        }
    }

    printf("\n*** Multilevel Aging Scheduler Completed ***\n");
    log_print("\n*** Multilevel Aging Scheduler Completed ***\n");
}