#include "./Config/types.h"
#include "./Config/config.h"
#include <stdio.h>
#include <stdlib.h>


char* PATH = "./Config/config.txt";

int main(void){
    Config* CFG = malloc(sizeof(Config));
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
        
    }
    
   return 0; 
}
