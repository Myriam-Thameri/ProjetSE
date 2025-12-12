#include "../Config/types.h"
#include "../Config/config.h"
#include "../Utils/Algorithms.h"
#include "../Utils/log_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/* ============================================================================
   SELECT SHORTEST JOB FIRST
   ============================================================================ */

PROCESS select_SJF(QUEUE queue) {
    QueueNode *node = queue.start;
    PROCESS minP = node->process;

    while (node != NULL) {
        if (node->process.execution_time < minP.execution_time) {
            minP = node->process;
        }
        node = node->next;
    }
    return minP;
}

/* ============================================================================
   SJF ALGORITHM — FINAL FIXED VERSION
   ============================================================================ */

void SJF_Algo(Config *config) {
    clear_gantt_slices();
    clear_io_slices();

    PCB *pcb = initialize_PCB(config);
    int time = 0;
    int finished = 0;

    // For ASCII Gantt
    char line1[2000] = "";
    char line2[2000] = "";
    char line3[2000] = "";
    char line4[2000] = "";

    QUEUE ready_queue = {NULL, NULL, 0};
    QUEUE io_queue = {NULL, NULL, 0};

    printf("SJF simulation start\n");

    PROCESS current;
    int cpu_busy = 0;

    while (finished < config->process_count) {

        printf("\nTime = %d\n", time);
        log_print("\nTime = %d\n", time);

        /* ---------------------------------------------------------------
           1. PROCESS ARRIVALS
           --------------------------------------------------------------- */
        for (int i = 0; i < config->process_count; i++) {
            PROCESS p = config->processes[i];

            if (p.arrival_time == time && !pcb[i].finished && !pcb[i].in_io) {
                printf("At time %d: Process %s arrived\n", time, p.ID);
                log_print("At time %d: Process %s arrived\n", time, p.ID);
                ready_queue = add_process_to_queue(ready_queue, p);
            }
        }

        /* ---------------------------------------------------------------
           2. IO MANAGEMENT - Process I/O BEFORE selecting/executing CPU
           --------------------------------------------------------------- */
        if (io_queue.size > 0) {
            PROCESS io_p = io_queue.start->process;

            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(io_p.ID, pcb[i].process.ID) == 0 && pcb[i].in_io) {

                    // Only decrement if io_remaining > 1, or if this is the last unit
                    if (pcb[i].io_remaining > 0) {
                        pcb[i].io_remaining--;

                        printf("At time %d: Process %s executes IO (%d left)\n", time, io_p.ID, pcb[i].io_remaining);
                        log_print("At time %d: Process %s executes IO (%d left)\n", time, io_p.ID, pcb[i].io_remaining);

                        if (pcb[i].io_remaining == 0) {
                            printf("At time %d: Process %s IO finished\n", time, io_p.ID);
                            log_print("At time %d: Process %s IO finished\n", time, io_p.ID);

                            pcb[i].in_io = 0;
                            pcb[i].io_index++;

                            io_queue = remove_specific_process(io_queue, io_p.ID);
                            ready_queue = add_process_to_queue(ready_queue, io_p);
                        }
                    }
                    break;
                }
            }
        }

        /* ---------------------------------------------------------------
           3. CPU SELECTION (NON-PREEMPTIVE SJF)
           --------------------------------------------------------------- */
        if (!cpu_busy && ready_queue.size > 0) {
            current = select_SJF(ready_queue);
            cpu_busy = 1;
            printf("At time %d: CPU selects %s (SJF)\n", time, current.ID);
            log_print("At time %d: CPU selects %s (SJF)\n", time, current.ID);
        }

        int cpu_executed = 0;

        /* ---------------------------------------------------------------
           4. CPU EXECUTION
           --------------------------------------------------------------- */
        if (cpu_busy) {
            for (int i = 0; i < config->process_count; i++) {
                if (strcmp(current.ID, pcb[i].process.ID) == 0 &&
                    !pcb[i].finished &&
                    !pcb[i].in_io) {

                    pcb[i].remaining_time--;
                    pcb[i].executed_time++;
                    cpu_executed = 1;

                    printf("At time %d: %s executes\n", time, current.ID);
                    log_print("At time %d: %s executes\n", time, current.ID);

                    add_gantt_slice(current.ID, time, 1, NULL);

                    strcat(line1, "--");
                    strcat(line2, current.ID);
                    strcat(line2, " ");
                    strcat(line3, "--");
                    strcat(line4, "   ");

                    /* ---- PROCESS FINISHED ---- */
                    if (pcb[i].remaining_time == 0) {
                        printf("At time %d: %s finishes\n", time, current.ID);
                        log_print("At time %d: %s finishes\n", time, current.ID);

                        pcb[i].finished = 1;
                        finished++;

                        ready_queue = remove_specific_process(ready_queue, current.ID);
                        cpu_busy = 0;
                        break;
                    }

                    /* ---- IO START ---- (Check AFTER finish check) */
                    if (current.io_count > 0 &&
                        pcb[i].io_index < current.io_count &&
                        pcb[i].executed_time ==
                            current.io_operations[pcb[i].io_index].start_time) {

                        printf("At time %d: %s starts IO\n", time + 1, current.ID);
                        log_print("At time %d: %s starts IO\n", time + 1, current.ID);
                        
                        // Add I/O slice starting at time + 1
                        add_io_slice(current.ID, time + 1,
                            current.io_operations[pcb[i].io_index].duration,
                            NULL, "I/O");

                        pcb[i].in_io = 1;
                        // Set io_remaining to duration + 1, because it will be decremented 
                        // immediately in the next iteration
                        pcb[i].io_remaining = current.io_operations[pcb[i].io_index].duration + 1;

                        ready_queue = remove_specific_process(ready_queue, current.ID);
                        io_queue = add_process_to_queue(io_queue, current);

                        cpu_busy = 0;
                        break;
                    }

                    break;
                }
            }
        }

        /* ---------------------------------------------------------------
           5. CPU IDLE
           --------------------------------------------------------------- */
        if (!cpu_executed) {
            add_gantt_slice("IDLE", time, 1, "#cccccc");

            strcat(line1, "--");
            strcat(line2, "   ");
            strcat(line3, "--");
            strcat(line4, "   ");
        }

        time++;
    }
    
    log_print("\n***SJF Algorithm Completed ***\n");
    
    printf("\nGantt Chart\n");
    printf("%s\n", line1);
    printf("%s\n", line2);
    printf("%s\n", line3);
    printf("%s\n", line4);
}