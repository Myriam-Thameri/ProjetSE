/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "./Config/types.h"
#include "./Config/config.h"
#include "./Algorithms/Algorithms.h"
#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[]){

    if (argc < 2) {
        printf("Usage: %s <config-file-path>\n", argv[0]);
        return 1;
    }

    char *PATH = argv[1];

    Config* CFG = malloc(sizeof(Config));

    if(CFG == NULL){
        printf("Memory allocation failed.\n");
        return 1;
    }

    int config_load_res;
    config_load_res = load_config(PATH , CFG);
    
    if(config_load_res == 0) {
        printf("Error loading config file.\n");
    }else if (config_load_res == 1) {
        for(int i=0; i<CFG -> process_count; i++){
            PROCESS p = CFG -> processes[i];
            printf("Process ID: %s\n", p.ID);
            printf("  Coming Time: %d\n", p.arrival_time);
            printf("  Execution Time: %d\n", p.execution_time);
            printf("  Priority: %d\n", p.priority);
            printf("  %s has %d IO Operations;\n",p.ID, p.io_count);
            for(int j=0; j<p.io_count; j++){
                IO_OPERATION io = p.io_operations[j];
                printf("    IO Operation %d: Start Time = %d, Duration = %d\n", j+1, io.start_time, io.duration);
            }
        }
        printf("Configuration loaded successfully.\n");
	int ans = -1;
	while(ans != 0){
        printf("Please choose your algorithm !\t");
	printf("Press 1 : FIFO\t");
	printf("Press 2 : Preemptive Priority\t");
	printf("Press 3 : Round Robin\t");
	printf("Press 4 : Shortest job first\t");
    printf("Press 5 : Multi level aging\t");
	printf("Press 6 : Multi level static\t");
	printf("Press 7 : Shot Remaining Time\t");
	printf("Or press 0 to quit :)");
	scanf("%d",&ans);
    switch(ans){
        case 1:
            FCFS_Algo(CFG);
            break;
        case 2:
            run_priority_preemptive(CFG->processes, CFG->process_count);
            break;
        case 3:
            RoundRobin_Algo(CFG);
            break;
        case 4:
            SJF_Algo(CFG);
            break;
        case 5:
            MultilevelAgingScheduler(CFG);
            break;
        case 6:
            MultilevelStaticScheduler(CFG);
            break;
        case 7:
            SRT_Algo(CFG);
            break;
        default:FCFS_Algo(CFG); break;

}}
    }
    free(CFG);
    return 0;
}
