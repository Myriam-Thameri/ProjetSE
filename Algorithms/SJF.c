#include "../Config/types.h"
#include "../Config/config.h"
#include "Algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




/* -------------------------------------
   SÉLECTION DU PROCESSUS SJF
-------------------------------------- */
PROCESS select_SJF(QUEUE queue) {

    QueueNode* node = queue.start;
    PROCESS minP = node->process;

    while (node != NULL) {
        if (node->process.execution_time < minP.execution_time) {
            minP = node->process;
        }
        node = node->next;
    }

    return minP;
}

void SJF_Algo(Config* config) {

    PCB* pcb = initialize_PCB(config);
    int time = 0;
    int finished = 0;

    char line1[2000] = "";
    char line2[2000] = "";
    char line3[2000] = "";
    char line4[2000] = "";

    QUEUE ready_queue = {0, NULL, NULL};
    QUEUE io_queue = {0, NULL, NULL};

    printf("PCB initialized for SJF (non-preemptive)\n");

    PROCESS current;
    int cpu_busy = 0;


    while (finished < config->process_count) {

        printf("\nTime = %d\n", time);

        /* -----------------------------
           PROCESSUS ARRIVÉS
        ------------------------------*/
        for (int i = 0; i < config->process_count; i++) {
            PROCESS p = config->processes[i];

            if (p.arrival_time == time && !pcb[i].finished && !pcb[i].in_io) {
                printf("At time %d: Process %s arrived\n", time, p.ID);
                ready_queue = add_process_to_queue(ready_queue, p);
            }
        }


        /* -----------------------------
              GESTION IO
        ------------------------------*/
        if (io_queue.size > 0) {

            PROCESS io_p = io_queue.start->process;

            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(io_p.ID, pcb[i].process.ID) == 0 && pcb[i].in_io) {

                    pcb[i].io_remaining--;

                    printf("At time %d: Process %s executes IO (%d left)\n",
                           time, io_p.ID, pcb[i].io_remaining);

                    if (pcb[i].io_remaining == 0) {
                        printf("At time %d: Process %s IO finished, returns to ready queue\n",
                               time, io_p.ID);

                        pcb[i].in_io = 0;
                        pcb[i].io_index++;

                        io_queue = remove_process_from_queue(io_queue);
                        ready_queue = add_process_to_queue(ready_queue, io_p);
                    }
                    break;
                }
            }
        }


        /* -----------------------------
               CPU SÉLECTION SJF
        ------------------------------*/
        if (!cpu_busy && ready_queue.size > 0) {

            current = select_SJF(ready_queue);
            cpu_busy = 1;

            printf("At time %d: CPU selects %s for execution (SJF)\n", time, current.ID);
        }


        int cpu_executed = 0;

        if (cpu_busy) {

            for (int i = 0; i < config->process_count; i++) {

                if (strcmp(current.ID, pcb[i].process.ID) == 0 &&
                    !pcb[i].finished && !pcb[i].in_io) {

                    pcb[i].remaining_time--;
                    pcb[i].executed_time++;
                    cpu_executed = 1;

                    printf("At time %d: %s executes\n", time, current.ID);

                    strcat(line1, "--");
                    strcat(line2, current.ID);
                    strcat(line2, " ");
                    strcat(line3, "--");
                    strcat(line4, "   ");

                    /* IO START ? */
                    if (current.io_count > 0 &&
                        pcb[i].io_index < current.io_count &&
                        pcb[i].executed_time ==
                            current.io_operations[pcb[i].io_index].start_time) {

                        printf("At time %d: %s starts IO\n", time, current.ID);

                        pcb[i].in_io = 1;
                        pcb[i].io_remaining =
                            current.io_operations[pcb[i].io_index].duration;

                        ready_queue = remove_process_from_queue(ready_queue);

                        io_queue = add_process_to_queue(io_queue, current);
                        cpu_busy = 0;
                        break;
                    }

                    /* FIN ? */
                    if (pcb[i].remaining_time == 0) {
                        printf("At time %d: %s finishes\n", time, current.ID);

                        pcb[i].finished = 1;
                        finished++;

                        ready_queue = remove_process_from_queue(ready_queue);
                        cpu_busy = 0;
                        break;
                    }

                    break;
                }
            }
        }

        /* CPU INACTIF */
        if (!cpu_executed) {
            strcat(line1, "--");
            strcat(line2, "   ");
            strcat(line3, "--");
            strcat(line4, "   ");
        }

        time++;
    }

    printf("\nGantt Chart\n");
    printf("%s\n", line1);
    printf("%s\n", line2);
    printf("%s\n", line3);
    printf("%s\n", line4);
}
