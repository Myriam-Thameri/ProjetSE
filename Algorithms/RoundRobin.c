#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
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

void add_process_to_queue(QUEUE* ready_queue, PROCESS p){
    printf("Adding process %s to the ready queue\n", p.ID);
    QueueNode* new_node = malloc(sizeof(QueueNode));
    new_node->process = p;
    new_node->next = NULL;

    if (ready_queue->size == 0){
        ready_queue->start = new_node;
        ready_queue->end = new_node;
    } else {
        ready_queue->end->next = new_node;
        ready_queue->end = new_node;
    }

    ready_queue->size++;
}

QUEUE remove_process_from_queue(QUEUE ready_queue){
    printf("Removing process %s from the ready queue\n", ready_queue.start->process.ID);
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

void RoundRobin_Algo(Config* config) {
    
    PCB* pcb;
    int time = 0;
    int finished = 0;
    char line1[1000]="";
    char line2[1000]="";
    char line3[1000]="";
    // ready queue represente la liste d'attente
    QUEUE ready_queue;
    ready_queue.start=NULL;
    ready_queue.end=NULL;
    ready_queue.size=0;

    // ready queue represente la liste d'attente
    QUEUE io_queue;
    io_queue.start=NULL;
    io_queue.end=NULL;
    io_queue.size=0;

    //Quantum
    int quantum;
    printf("Enter Quantum Time: ");
    scanf("%d", &quantum);
    printf("Quantum Time set to %d units\n", quantum);
    int used_quantum=0;

    // a variable to check if we can use the cpu or not
    int cpu_busy = 0;
    //Initialize PCBs
    pcb = initialize_PCB(config);
    printf("pcb initialized\n");

    while(finished < config->process_count){
        for (int i=0; i<config->process_count; i++){
            PROCESS p = config->processes[i];
            
            // processnot yet arrived
            if (p.arrival_time > time){
                continue;
            }

            //process already finished
            if(pcb[i].finished && strcmp(ready_queue.start->process.ID , p.ID) != 0){
                continue;
            }


            // process just arrived 
            if (p.arrival_time == time){
                printf("Process %s has arrived and is added to the ready queue\n", p.ID);
                add_process_to_queue(&ready_queue, p);
                continue;
            }             


            //process finishes now
            if(pcb[i].remaining_time==0 && strcmp(ready_queue.start->process.ID , p.ID) == 0){
                pcb[i].finished=1;
                finished++;
                ready_queue = remove_process_from_queue(ready_queue);
                used_quantum=0;
                printf("Process %s has finished execution at time %d\n", p.ID, time);
                snprintf(line3 + strlen(line3), sizeof(line3) - strlen(line3), "at time %d process %s is added to the io queue\n", time, p.ID);
                strcat(line1,"-- |");
                strcat(line2, p.ID);
                cpu_busy=0;
                continue;
            }

            //process finishes its quantum now
            if(pcb[i].remaining_time!=0 && strcmp(ready_queue.start->process.ID , p.ID) == 0 && used_quantum==quantum){ {
                
                ready_queue = remove_process_from_queue(ready_queue);
                add_process_to_queue(&ready_queue, p);
                cpu_busy=0;
                used_quantum=0;
                
                continue;
            }


            //process in io 
            if(pcb[i].in_io){
                //check if it is waiting to execute the io or it is the one executing
                if (strcmp(io_queue.start->process.ID , p.ID) == 0){//it is the one executing
                    //decrease the remaining time by one unit
                    pcb[i].io_remaining--;
                    //if the io finish at this time
                    if(pcb[i].io_remaining == 0){
                        //remove from io queue
                        io_queue = remove_process_from_queue(io_queue);
                        //Exit the io and move to the next one
                        pcb[i].in_io = 0;
                        pcb[i].io_index++;
                        //add the process to the ready queue
                        printf("Process %s finished IO and is added back to the ready queue\n", p.ID);
                        add_process_to_queue(&ready_queue , p);
                        
                    }
                }
                
                continue;
            }

           
            //execute it in cpu
            if(ready_queue.size >0){

                //check if it is the one to be executed
                //if the id of the first process in the ready queue is equal to the current process id
                if(strcmp(ready_queue.start->process.ID , p.ID) == 0){
                    cpu_busy=1;
                    if (used_quantum < quantum && pcb[i].executed_time < p.execution_time)
                    {
                        used_quantum++;
                        //execute
                        pcb[i].executed_time ++;
                        strcat(line1,"-- ");
                        strcat(line2, "   ");
                    }

                }
                 //if it will start an io
                if( p.io_count >0 ){// if it has io operations
                    if (pcb[i].io_index < p.io_count && pcb[i].executed_time == p.io_operations[pcb[i].io_index].start_time){
                        pcb[i].in_io = 1;
                        pcb[i].io_remaining = p.io_operations[pcb[i].io_index].duration;
                        add_process_to_queue(&io_queue , p);
                        ready_queue = remove_process_from_queue(ready_queue);
                        
                        snprintf(line3 + strlen(line3), sizeof(line3) - strlen(line3), "at time %d process %s is added to the io queue\n", time, p.ID);
                        strcat(line1,"-- |");
                        strcat(line2, p.ID);
                        break;
                    }
                }
                continue;
            }
            //execute the io 
            if(io_queue.size >0){

                //check if it has io to execute 
                if (pcb[i].in_io){
                    
                    //check if it is the one to be executed
                    if(strcmp(ready_queue.start->process.ID , p.ID) == 0){
                        
                    //decrease the remaining time by one unit
                        pcb[i].io_remaining--;
                        //if the io finish at this time
                        if(pcb[i].io_remaining == 0){
                            //remove from io queue
                            io_queue = remove_process_from_queue(io_queue);
                            //Exit the io and move to the next one
                            pcb[i].in_io = 0;
                            pcb[i].io_index++;
                            //add the process to the ready queue
                            printf("Process %s finished IO and is added back to the ready queue\n", p.ID);
                            add_process_to_queue(&ready_queue , p);
                            
                        }
                        continue;

                    }
                }
            
            }
        }            
        time++;
        printf("Time %d:\n", time);
        printf("%s\n", line1);
        printf("%s\n", line2);
        printf("%s\n", line3); 
    }
    
    }   
    printf("Gantt Chart:\n");
    printf("%s\n", line1);
    printf("%s\n", line2);
    printf("%s\n", line3);
}
