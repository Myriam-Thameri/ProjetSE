#include <stdio.h>
#include "config.h"

int main(){
	FILE *f = fopen("config.txt","r");
	if (!f){
		perror("Erreur");
		return 1;
	}

	PROCESS processes[50];
	int count = 0;
	while(!feof(f)){
	PROCESS p;
	fscanf(f,"%d %d %d %d %d", &p.id, &p.arrival_time, &p.execution_time, &p.priority, &p.io_count);
	
	for(int i=0;i<p.io_count;i++){
		fscanf(f,"%d %d", &p.io_operations[i].start_time, &p.io_operations[i].duration);
	}
	processes[count++] = p;
	}

	fclose(f);

	for(int i=0;i <count - 1; i++){
	printf("PROCESS %d: \n", processes[i].id);
	printf("Arrival time: %d \n", processes[i].arrival_time);
	printf("Execution time: %d \n", processes[i].execution_time);
	printf("Priority: %d \n", processes[i].priority);
	printf("IO count: %d \n", processes[i].io_count);
		for(int j=0;j<processes[i].io_count; j++){
			IO_OPERATION io = processes[i].io_operations[j];
			printf(" IO start at t = %d for %d units \n", io.start_time, io.duration);
}
}
return 0;
}

