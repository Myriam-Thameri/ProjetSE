/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "./Config/types.h"
#include "./Config/config.h"
#include "./Utils/Algorithms.h"
#include "./Utils/utils.h"
#include "./Interface/interface_utils.h"

#define MAX_FILES 50
#define MAX_FILENAME_LEN 20

char* DIR_PATH = "./Config";

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
            PROCESS p = CFG->processes[i];
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
            run_priority_preemptive(CFG);
            break;
        case 3:
            RoundRobin_Algo(CFG);
            break;
        case 4:
            SJF_Algo(CFG);
            break;
        case 5: {
            int quantum = 2;
            int aging_interval = 5;
            int max_priority = 5;

            printf("\nEnter Quantum (default 2): ");
            scanf("%d", &quantum);
            if (quantum <= 0) quantum = 2;

            printf("Enter Aging Interval (default 5): ");
            scanf("%d", &aging_interval);
            if (aging_interval <= 0) aging_interval = 5;

            printf("Enter Max Priority (default 5): ");
            scanf("%d", &max_priority);
            if (max_priority <= 0) max_priority = 5;

            MultilevelAgingScheduler(CFG, quantum, aging_interval, max_priority);
            break;
        }
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
 */
int main(int argc, char *argv[]){
    
    AppContext *app_data = g_new0(AppContext, 1);
    app_data->CFG = g_new0(Config, 1); 

    GtkApplication *app = gtk_application_new("com.example.OSScheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_free(app_data->CFG);
    g_free(app_data);
    g_object_unref(app);
    return status;
    
}

