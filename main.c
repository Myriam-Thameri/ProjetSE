#include "./Config/types.h"
#include "./Config/config.h"
#include "./Algorithms/Algorithms.h"

#include <stdio.h>
#include <stdlib.h>

	@@ -12,7 +8,7 @@ int main(void){
    Config* CFG = malloc(sizeof(Config));
    int config_load_res;
    config_load_res = load_config(PATH , CFG);
    
    if(config_load_res == 0) {
        printf("Error loading config file.\n");
    }else if (config_load_res == 1) {
	@@ -34,8 +30,8 @@ int main(void){
        printf("Testing Multilevel Scheduler with Aging...\n");
        MultilevelAgingScheduler(CFG);

        
    }
    
   return 0;
}