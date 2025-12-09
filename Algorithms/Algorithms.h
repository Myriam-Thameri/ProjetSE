#ifndef ALGORITHMS_H

#define ALGORITHMS_H

#include "../Config/config.h"
#include "../Config/types.h"

typedef struct node node_t;
typedef struct SRT_Scheduler SRT_Scheduler;

SRT_Scheduler* SRT_create();

void SRT_add_process(SRT_Scheduler *s, PROCESS *p);

PROCESS* SRT_next(SRT_Scheduler *s);


void SRT_destroy(SRT_Scheduler *s);

void SRT_Algo(Config* config);

void MultilevelAgingScheduler(Config* config);

void run_priority_preemptive(Config* config);

void SJF_Algo(Config* config);

void RoundRobin_Algo(Config* config);

PCB* initialize_PCB(Config* config); //return an initialized PCB for each process in a list 

QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p); // add a node in the end of the ready queue and then return it 

QUEUE remove_process_from_queue(QUEUE ready_queue); //remove the first process in the queue and return the updated queue


void MultilevelStaticScheduler(Config* config);

#endif // ALGORITHMS_H