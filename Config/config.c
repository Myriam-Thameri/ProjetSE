#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "types.h"

void trim(char* s) {
    // remove spaces in the start
    char *start = s;
    while(*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')
        start++;
    memmove(s, start, strlen(start)+1);  // shift string left

    // remove spaces in the end
    char* end = s + strlen(s) - 1;
    while(end > s && (*end==' ' || *end=='\t' || *end=='\n' || *end=='\r'))
        *end-- = '\0';
}

int load_config(char* path , Config* cfg) {
    FILE* file;
    if ((file = fopen(path, "r")) == NULL) {
        perror("fopen()");
        return 0;
    } //if the file cannot be opened the prgrm return NULL
    
    
    char line[256];
    int process = -1;
    int p_io=-1;
     // line by line till the end
    while(fgets(line, sizeof(line), file)) {
        printf(" %s \n", line);
        trim(line);
        if (line[0] == '#' || strlen(line) ==0 ) {
            continue;
        } // ignore comments(the comments begin with #) and empty lines
        
        
        if (line[0] == '[') {
            char section[20]; // process or process_io
            sscanf(line, "[%[^]]]", section); //extract the name between []
            
            //in case it is a process
            if (strncmp(section, "process",7)==0 && strchr(section, '_')==NULL){
                sscanf( section, "process%d", &process);
                p_io=-1;
            }
            //in case it is a process io
            else if (strncmp(section, "process_io",10)==0 && strchr(section, '_')!=NULL){
                sscanf( section, "process%d_io%d", &process, &p_io);
                p_io++;
                
            }
            continue; //continue to their attributes
        }

        char* eq = strchr(line, '=');
        if (eq != NULL) {
            *eq = '\0';
            char* key = line;
            char* value = eq + 1;

            //eliminate the white spaces
            trim(key);
            trim(value);
            //config   att
            if (strcmp(key, "process_count") == 0) { // key=process_count
                cfg ->process_count = atoi(value);
                printf("process count : %d\n", cfg ->process_count);
                continue;
            }

            //process att
            if(p_io == -1){
                if (strcmp(key , "ID")==0){
                    strcpy(cfg -> processes[process].ID , value);
                }
                else if (strcmp(key , "arrival_time")==0){
                    cfg -> processes[process].arrival_time = atoi(value);
                }
                else if (strcmp(key , "execution_time")==0){
                    cfg -> processes[process].execution_time = atoi(value);
                }
                else if (strcmp(key , "priority")==0){
                    cfg -> processes[process].priority = atoi(value);
                }
                else if (strcmp(key , "io_count")==0){
                    cfg -> processes[process].io_count = atoi(value);
                }
            }

            //io att
            else {
                
            printf("Key: '%s', Value: '%s' p_io :'%d'\n", key, value,p_io);
                if (strcmp(key , "start_time")==0){
                    cfg -> processes[process].io_operations[p_io].start_time = atoi(value);
                }
                else if (strcmp(key , "duration")==0){
                    cfg -> processes[process].io_operations[p_io].duration = atoi(value);
                }
            }

        } 
    }
    fclose(file);
    return 1;
}