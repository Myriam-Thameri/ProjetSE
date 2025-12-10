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


void RoundRobin_Algo(Config* config) {
    
    PCB* pcb = initialize_PCB(config);
    int time = 0;
    int finished = 0;
    char line1[2000] = "";
    char line2[2000] = "";
    char line3[2000] = "";
    char line4[2000] = "";
    
    QUEUE ready_queue ;
    ready_queue.size = 0;
    ready_queue.start = NULL;
    ready_queue.end = NULL; 
    QUEUE io_queue ;
    io_queue.size = 0;
    io_queue.start = NULL;
    io_queue.end = NULL;

    int quantum;
    printf("Enter Quantum Time: ");
    scanf("%d", &quantum);
    printf("Quantum Time set to %d units\n", quantum);
    int used_quantum = 0;

    printf("PCB initialized\n");

    while(finished < config->process_count) {
        printf("\nTime = %d \n", time);
        
        // get the just arrived processes
        for (int i = 0; i < config->process_count; i++) {
            PROCESS p = config->processes[i];
            
            if (p.arrival_time == time && !pcb[i].finished && !pcb[i].in_io) { // add to ready queue
                printf("At time %d: Process %s arrived and added to ready queue\n", time, p.ID);
                ready_queue = add_process_to_queue(ready_queue, p);
            }
        }

        // IO 
        if (io_queue.size > 0) {
            PROCESS io_p = io_queue.start->process;
            
            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(pcb[i].process.ID, io_p.ID) == 0 && pcb[i].in_io) {
                    pcb[i].io_remaining--;
                    printf("At time %d: Process %s executes its IO and it rest : %d\n", time, io_p.ID, pcb[i].io_remaining);
                    
                    if (pcb[i].io_remaining == 0) {
                        io_queue = remove_process_from_queue(io_queue);
                        pcb[i].in_io = 0;
                        pcb[i].io_index++;
                        printf("At time %d: Process %s finished IO & added back to ready queue\n", time, io_p.ID);
                        ready_queue = add_process_to_queue(ready_queue, io_p);
                    }
                    break;
                }
            }
        }

        // process 
        int cpu_executed = 0;
        if (ready_queue.size > 0) {
            PROCESS p = ready_queue.start->process;
            
            for (int i = 0; i < config->process_count; i++) {// search the process in PCB
                if (strcmp(pcb[i].process.ID, p.ID) == 0 && !pcb[i].finished && !pcb[i].in_io) {
                    
                    pcb[i].executed_time++;
                    pcb[i].remaining_time--;
                    used_quantum++;
                    cpu_executed = 1;
                    
                    printf("At time %d: Process %s executs\n", time, p.ID);
                    
                    strcat(line1, "--");
                    strcat(line2, "   ");
                    strcat(line3, "--");
                    strcat(line4, "   ");
                    
                    if (p.io_count > 0 && pcb[i].io_index < p.io_count && pcb[i].executed_time == p.io_operations[pcb[i].io_index].start_time) {
                        
                        printf("At time %d: Process %s starts IO\n", time, p.ID);
                        pcb[i].in_io = 1;
                        pcb[i].io_remaining = p.io_operations[pcb[i].io_index].duration;
                        ready_queue = remove_process_from_queue(ready_queue);
                        io_queue = add_process_to_queue(io_queue, p);
                        used_quantum = 0;
                        strcat(line2, p.ID);
                        strcat(line2, "|");
                        snprintf(line4 + strlen(line4), sizeof(line4) - strlen(line4),  "%d", time + 1);
                    }
                    //  if finished
                    else if (pcb[i].remaining_time == 0) {
                        printf("At time %d: Process %s finishes\n", time, p.ID);
                        pcb[i].finished = 1;
                        finished++;
                        ready_queue = remove_process_from_queue(ready_queue);
                        used_quantum = 0;
                        
                        strcat(line2, p.ID);
                        strcat(line2, " | ");
                        snprintf(line4 + strlen(line4), sizeof(line4) - strlen(line4), "%d", time + 1);
                    }
                    // quantum finished
                    else if (used_quantum >= quantum) {
                        printf("At time %d: Process %s quantum finish\n", time, p.ID);
                        ready_queue = remove_process_from_queue(ready_queue);
                        ready_queue = add_process_to_queue(ready_queue, p);
                        used_quantum = 0;
                        
                        strcat(line2, p.ID);
                        strcat(line2, " | ");
                        snprintf(line4 + strlen(line4), sizeof(line4) - strlen(line4), "%d", time + 1);
                    }
                    
                    break;
                }
            }
        }
        
        // CPU idle
        if (!cpu_executed) {
            strcat(line1, "--");
            strcat(line2, "   ");
            strcat(line3, "--");
            strcat(line4, "   ");
        }
        
        time++;
    }
    
    printf("\nGantt Chart \n");
    printf("%s\n", line1);
    printf("%s\n", line2);
    printf("%s\n", line3);
    printf("%s\n", line4);
}
