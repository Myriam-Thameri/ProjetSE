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
#include "../Utils/Algorithms.h"
#include "../Utils/log_file.h"
#include "../Config/config.h"
#include "../Config/types.h"
#include "../Interface/gantt_chart.h"
#include "../Interface/gantt_chart.h"

void SRT_Algo(Config* config) {
    clear_gantt_slices();
    clear_io_slices();
    
    int n = config->process_count;
    if (n <= 0) {
        printf("No processes to schedule.\n");
        return;
    }

    int *remaining = malloc(sizeof(int) * n);
    int *executed = malloc(sizeof(int) * n);
    int *next_io = malloc(sizeof(int) * n);
    int *blocked = malloc(sizeof(int) * n);
    int *finished = malloc(sizeof(int) * n);
    int *started = malloc(sizeof(int) * n);
    int *start_time = malloc(sizeof(int) * n);
    int *end_time = malloc(sizeof(int) * n);
    int *response_time = malloc(sizeof(int) * n);
    int *io_started_this_tick = malloc(sizeof(int) * n);

    if (!remaining || !executed || !next_io || !blocked || !finished ||
        !started || !start_time || !end_time || !response_time || !io_started_this_tick) {
        fprintf(stderr, "Memory allocation failed in SRT_Algo\n");
        exit(1);
    }

    int total_exec = 0;
    int total_io = 0;
    for (int i = 0; i < n; ++i) {
        remaining[i] = config->processes[i].execution_time;
        executed[i] = 0;
        next_io[i] = 0;
        blocked[i] = 0;
        finished[i] = 0;
        started[i] = 0;
        start_time[i] = -1;
        end_time[i] = -1;
        response_time[i] = -1;
        io_started_this_tick[i] = 0;
        total_exec += config->processes[i].execution_time;
        for (int j = 0; j < config->processes[i].io_count; ++j)
            total_io += config->processes[i].io_operations[j].duration;
    }

    int max_ticks = total_exec + total_io + 1000;
    int processes_left = n;
    int tick = 0;

    printf("=== Running SRT Algorithm (preemptive) with I/O ===\n");

    while (processes_left > 0 && tick < max_ticks) {
        for (int i = 0; i < n; ++i) {
            if (blocked[i] > 0 && !io_started_this_tick[i]) {
                blocked[i]--;
                if (blocked[i] == 0) {
                    printf("Time %d: Process %s finished I/O and is READY again (remaining %d)\n", 
                           tick, config->processes[i].ID, remaining[i]);
                    log_print("Time %d: Process %s finished I/O and is READY again (remaining %d)\n", 
                             tick, config->processes[i].ID, remaining[i]);
                }
            }
            io_started_this_tick[i] = 0; 
        }

        int shortest = -1;
        for (int i = 0; i < n; ++i) {
            if (finished[i]) continue;
            if (blocked[i] > 0) continue;
            if (config->processes[i].arrival_time > tick) continue;
            if (remaining[i] <= 0) continue;

            if (shortest == -1
                || remaining[i] < remaining[shortest]
                || (remaining[i] == remaining[shortest] && 
                    config->processes[i].arrival_time < config->processes[shortest].arrival_time)) {
                shortest = i;
            }
        }

        if (shortest == -1) {
            printf("Time %d: CPU idle\n", tick);
            log_print("Time %d: CPU idle\n", tick);
            add_gantt_slice("IDLE", tick, 1, "#cccccc");
            tick++;
            continue;
        }

        PROCESS *p = &config->processes[shortest];

        if (!started[shortest]) {
            started[shortest] = 1;
            start_time[shortest] = tick;
            response_time[shortest] = tick - p->arrival_time;
        }

        printf("Time %d: Running %s (Remaining %d)\n", tick, p->ID, remaining[shortest]);
        log_print("Time %d: Running %s (Remaining %d)\n", tick, p->ID, remaining[shortest]);
        
        add_gantt_slice(p->ID, tick, 1, NULL);
        
        remaining[shortest]--;
        executed[shortest]++;

        if (remaining[shortest] <= 0) {
            finished[shortest] = 1;
            end_time[shortest] = tick + 1;
            processes_left--;
            printf("Time %d: Process %s FINISHED\n", tick + 1, p->ID);
            log_print("Time %d: Process %s FINISHED\n", tick + 1, p->ID);
            tick++;
            continue;
        }

        if (next_io[shortest] < p->io_count) {
            IO_OPERATION *io = &p->io_operations[next_io[shortest]];
            
            if (executed[shortest] == io->start_time) {
                blocked[shortest] = io->duration;
                next_io[shortest]++;
                io_started_this_tick[shortest] = 1; 
                
                add_io_slice(p->ID, tick + 1, io->duration, NULL, "I/O");
                
                printf("Time %d: Process %s goes to I/O for %d ticks\n", 
                       tick + 1, p->ID, io->duration);
                log_print("Time %d: Process %s goes to I/O for %d ticks\n", 
                         tick + 1, p->ID, io->duration);
            }
        }

        tick++;
    }

    if (tick >= max_ticks) {
        printf("Error: Simulation exceeded time limit (max_ticks=%d).\n", max_ticks);
        log_print("Error: Simulation exceeded time limit (max_ticks=%d).\n", max_ticks);
    }

    printf("\n=== SRT Final Results ===\n");
    printf("Process  Arrival  Burst  Completion  Turnaround  Waiting  Response\n");
    double sum_turn=0.0, sum_wait=0.0, sum_resp=0.0;
    for (int i = 0; i < n; ++i) {
        int arrival = config->processes[i].arrival_time;
        int burst = config->processes[i].execution_time;
        int comp = end_time[i] >= 0 ? end_time[i] : 0;
        int turnaround = (end_time[i] >= 0) ? (end_time[i] - arrival) : 0;
        int waiting = (turnaround > 0) ? (turnaround - burst) : 0;
        int resp = (response_time[i] >= 0) ? response_time[i] : -1;

        printf("%-8s %-7d %-6d %-10d %-11d %-8d %-8d\n",
               config->processes[i].ID, arrival, burst, comp, turnaround, waiting, resp);

        sum_turn += turnaround;
        sum_wait += waiting;
        sum_resp += (resp >= 0) ? resp : 0;
    }

    double avg_turn = sum_turn / n;
    double avg_wait = sum_wait / n;
    double avg_resp = sum_resp / n;

    printf("\n*** Short Remaining Time Finished ***\n");
    log_print("\n*** Short Remaining Time Finished ***\n");

    printf("\nAverage Turnaround Time: %.2f\n", avg_turn);
    printf("Average Waiting Time: %.2f\n", avg_wait);
    printf("Average Response Time: %.2f\n", avg_resp);

    free(remaining);
    free(executed);
    free(next_io);
    free(blocked);
    free(finished);
    free(started);
    free(start_time);
    free(end_time);
    free(response_time);
    free(io_started_this_tick);
}