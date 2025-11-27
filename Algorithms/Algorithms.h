#ifndef SRT_H
#define SRT_H

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
void run_priority_preemptive(PROCESS p[], int count);
