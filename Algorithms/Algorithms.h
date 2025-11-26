#ifndef ALGORITHMS_H

#define ALGORITHMS_H

#include "../Config/types.h"

void RoundRobin_Algo(Config* config);

void MultilevelStaticScheduler(Config* config);

PCB* initialize_PCB(Config* config); //return an initialized PCB for each process in a list 

void add_process_to_queue(QUEUE* ready_queue, PROCESS p); // add a node in the end of the ready queue and then return it 

QUEUE remove_process_from_queue(QUEUE ready_queue); //remove the first process in the queue and return the updated queue

#endif // ALGORITHMS_H
