/*
 * Simulateur d'Ordonnancement de Processus
 * Multilevel Scheduler with Dynamic Aging + I/O + Gantt Chart
 */

#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TIME 500

/* =========================
   GANTT CHART STRUCTURES
   ========================= */

typedef struct {
    char pid[5];
    int start;
    int end;
} GanttBlock;

GanttBlock gantt[MAX_TIME];
int gantt_size = 0;

void add_gantt_block(const char* pid, int start, int end) {
    if (gantt_size > 0 &&
        strcmp(gantt[gantt_size - 1].pid, pid) == 0 &&
        gantt[gantt_size - 1].end == start) {

        gantt[gantt_size - 1].end = end;
    } else {
        strcpy(gantt[gantt_size].pid, pid);
        gantt[gantt_size].start = start;
        gantt[gantt_size].end = end;
        gantt_size++;
    }
}

void print_gantt_chart() {
    printf("\n==================== GANTT CHART ====================\n");

    for (int i = 0; i < gantt_size; i++) {
        printf("| %s ", gantt[i].pid);
    }
    printf("|\n");

    for (int i = 0; i < gantt_size; i++) {
        printf("%-6d", gantt[i].start);
    }
    printf("%d\n", gantt[gantt_size - 1].end);

    printf("=====================================================\n");
}

/* =========================
   AGING MECHANISM
   ========================= */

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
        }
    }
}

/* =========================
   MAIN SCHEDULER
   ========================= */

void MultilevelAgingScheduler(Config* config,
                              int quantum,
                              int aging_interval,
                              int max_priority)
{
    PCB* pcbs = initialize_PCB(config);
    int total = config->process_count;
    int finished = 0;
    int time = 0;

    gantt_size = 0;

    printf("\n=== MULTILEVEL SCHEDULER WITH AGING + I/O ===\n");
    printf("Quantum=%d | Aging Interval=%d | Max Priority=%d\n\n",
           quantum, aging_interval, max_priority);

    while (finished < total)
    {
        /* =========================
           UPDATE I/O
           ========================= */
        for (int i = 0; i < total; i++) {
            PCB* p = &pcbs[i];

            if (p->in_io) {
                p->io_remaining--;

                if (p->io_remaining <= 0) {
                    p->in_io = 0;
                    p->io_index++;
                    printf("Time %d: %s finished I/O and returned to READY\n",
                           time, p->process.ID);
                }
            }
        }

        /* =========================
           SELECT HIGHEST PRIORITY READY PROCESS
           ========================= */
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

        /* =========================
           APPLY AGING
           ========================= */
        apply_aging(pcbs, total, time, next,
                    aging_interval, max_priority);

        /* =========================
           EXECUTION
           ========================= */
        if (next) {
            int run_for = quantum;
            if (next->remaining_time < run_for)
                run_for = next->remaining_time;

            printf("Time %d: Scheduler selects %s (priority=%d)\n",
                   time, next->process.ID, next->process.priority);

            for (int t = 0; t < run_for; t++) {
                add_gantt_block(next->process.ID, time + t, time + t + 1);

                next->remaining_time--;
                next->executed_time++;

                /* =========================
                   CHECK FOR I/O START
                   ========================= */
                if (next->io_index < next->process.io_count) {
                    IO_OPERATION io =
                        next->process.io_operations[next->io_index];

                    if (next->executed_time == io.start_time) {
                        next->in_io = 1;
                        next->io_remaining = io.duration;

                        printf("Time %d: %s BLOCKED for I/O (%d units)\n",
                               time + t + 1,
                               next->process.ID,
                               io.duration);
                        break;
                    }
                }
            }

            next->wait_time = 0;
            time += run_for;

            /* =========================
               FINISH PROCESS
               ========================= */
            if (next->remaining_time <= 0) {
                next->finished = 1;
                finished++;
                printf("Time %d: %s has COMPLETED execution\n",
                       time, next->process.ID);
            }

        } else {
            printf("Time %d: CPU is IDLE\n", time);
            add_gantt_block("IDLE", time, time + 1);
            time++;
        }
    }

    print_gantt_chart();
}
