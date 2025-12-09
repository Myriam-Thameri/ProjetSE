
#include "../Config/types.h"
#include "../Config/config.h"
#include "./Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


PCB* initialize_PCB(Config* config) {
    static PCB pcb[50];
    for (int i = 0; i < config->process_count; i++)
    {
        pcb[i].process = config->processes[i];
        pcb[i].remaining_time =config->processes[i].execution_time;
        pcb[i].executed_time=0;
        pcb[i].io_index=0;
        pcb[i].in_io=0;
        pcb[i].io_remaining=0;
        pcb[i].finished=0;
    }
    return pcb;
}

QUEUE add_process_to_queue(QUEUE ready_queue, PROCESS p){
    
    QueueNode* new_node = malloc(sizeof(QueueNode));
    new_node->process = p;
    new_node->next = NULL;

    if (ready_queue.size == 0){
        ready_queue.start = new_node;
        ready_queue.end = new_node;
    } else {
        ready_queue.end->next = new_node;
        ready_queue.end = new_node;
    }

    ready_queue.size +=1;
    return ready_queue;
}

QUEUE remove_process_from_queue(QUEUE ready_queue){
    // Queue is empty nothing to remove retirn as is
    if (ready_queue.size == 0) {
        return ready_queue; 
    }
    // Remove the first node
    QueueNode* temp = ready_queue.start;
    ready_queue.start = ready_queue.start->next;
    free(temp);
    ready_queue.size--;
    
    return ready_queue;
}

PCB* find_pcb_by_id(PCB* pcb, int count, const char* id) {
    for (int i = 0; i < count; i++) {
        if (strcmp(pcb[i].process.ID, id) == 0) {
            return &pcb[i];
        }
    }
    return NULL;
}

