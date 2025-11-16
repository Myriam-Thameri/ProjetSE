#include <stdio.h>
typedef struct{
	int pid;
	int arrival_time;
	int burst_time;
	int remaining_time;
	int priority;
	int finish_time;
}Process;

int main(){
	int n=4;
	Process p[] = {
		{1,0,8,8,2,0},
		{2,1,4,4,1,0},
		{3,2,9,9,3,0},
		{4,3,5,5,2,0}
	};
	int completed = 0;
	int time = 0;
	while (completed < n){
		int idx=-1;
		int highest_priority = 9999;
		
		for(int i=0; i < n; i++){
			//Test if current process could start or has still not finished
			if(p[i].arrival_time <= time && p[i].remaining_time > 0){
				//Test if it has the lowest priority (lowest priority means you got the highest priority and could work
				if(p[i].priority < highest_priority){
					//lowest priority start
					highest_priority = p[i].priority;
					//asign the process id to idx
					idx = i;
					}
				//If the current process has the same proirity as the highest priority and his arrival time is less than the current process then we should basically assign it to the process that just came
				else if (p[i].priority == highest_priority && p[i].arrival_time < p[idx].arrival_time){
					idx = i;
				}
			}
		}

		if(idx == -1){
			//if idx is still -1 then we dont have any process working
			time++;
			continue;
		}

		p[idx].remaining_time--;
		time++;
		
		//we are going to check if current process has finished running, if yes then we assign its finish time and increment completed
		if (p[idx].remaining_time == 0){
			p[idx].finish_time = time;
			completed++;
		}
}

		printf("PID\tArrival\tBurst\tPriority\tFinish\n");
		for(int i=0;i<n;i++){
			printf("%d\t%d\t%d\t%d\t%d\n",p[i].pid,p[i].arrival_time,p[i].burst_time,p[i].priority, p[i].finish_time);
		}

		return 0;
}


