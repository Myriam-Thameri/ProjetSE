/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université de Tunis El Manar
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#ifndef ALGORITHMS_H

#define ALGORITHMS_H

#include "../Config/config.h"
#include "../Config/types.h"

typedef struct node node_t;
typedef struct SRT_Scheduler SRT_Scheduler;

SRT_Scheduler* SRT_create();

void SRT_add_process(SRT_Scheduler *s, PROCESS *p);

PROCESS* SRT_next(SRT_Scheduler *s);
PCB* find_pcb_by_id(PCB* pcb, int count, const char* id);

void SRT_destroy(SRT_Scheduler *s);

void SRT_Algo(Config* config);

void MultilevelAgingScheduler(Config* config, int quantum, int aging_interval, int max_priority);

void run_priority_preemptive(Config* config);

void SJF_Algo(Config* config);

void RoundRobin_Algo(Config* config, int quantum);

PCB* initialize_PCB(Config* config); 

QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p); 

QUEUE remove_process_from_queue(QUEUE ready_queue); 

QUEUE remove_specific_process(QUEUE q, const char *ID) ;

void FCFS_Algo(Config* config);

void MultilevelStaticScheduler(Config* config, int quantum);

#endif // ALGORITHMS_H
