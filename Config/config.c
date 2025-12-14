/*
 * Simulateur d'Ordonnancement de Processus 
 * Copyright (c) 2025 Équipe ProjetSE - Université de Tunis El Manar
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "types.h"

void trim(char* s) {
    
    char *start = s;
    while(*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')
        start++;
    memmove(s, start, strlen(start)+1);  

    char* end = s + strlen(s) - 1;
    while(end > s && (*end==' ' || *end=='\t' || *end=='\n' || *end=='\r'))
        *end-- = '\0';
}

int load_config(char* path , Config* cfg) {
    FILE* file;
    if ((file = fopen(path, "r")) == NULL) {
        perror("fopen()");
        return 0;
    } 
    
    char line[256];
    int process = -1;
    int p_io=-1;
     
    while(fgets(line, sizeof(line), file)) {

        trim(line);
        if (line[0] == '#' || strlen(line) ==0 ) {
            continue;
        } 
        
        if (line[0] == '[') {
            char section[20]; 
            sscanf(line, "[%[^]]]", section); 
            
            if (strncmp(section, "process",7)==0 && strchr(section, '_')==NULL){
                sscanf( section, "process%d", &process);
                p_io=-1;
            }
            else if (strncmp(section, "process_io",10)==0 && strchr(section, '_')!=NULL){
                sscanf( section, "process%d_io%d", &process, &p_io);
                p_io++;
                
            }
            continue; 
        }

        char* eq = strchr(line, '=');
        if (eq != NULL) {
            *eq = '\0';
            char* key = line;
            char* value = eq + 1;

            trim(key);
            trim(value);

            if (strcmp(key, "process_count") == 0) { 
                cfg ->process_count = atoi(value);
                continue;
            }

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
void free_config(Config *cfg)
{
    if (!cfg) return;
    free(cfg);
}

int save_config(char* path, Config* cfg) {
    if (!path || !cfg) return 0;

    FILE *file = fopen(path, "w");
    if (!file) return 0;

    fprintf(file, "#config file\n\n");
    fprintf(file, "process_count = %d\n\n", cfg->process_count);

    for (int i = 0; i < cfg->process_count; i++) {
        PROCESS *p = &cfg->processes[i];
        fprintf(file, "[process%d]\n", i);
        fprintf(file, "ID = %s\n", p->ID);
        fprintf(file, "arrival_time = %d\n", p->arrival_time);
        fprintf(file, "execution_time = %d\n", p->execution_time);
        fprintf(file, "priority = %d\n", p->priority);
        fprintf(file, "io_count = %d\n\n", p->io_count);

        for (int j = 0; j < p->io_count; j++) {
            fprintf(file, "[process%d_io%d]\n", i, j);
            fprintf(file, "start_time = %d\n", p->io_operations[j].start_time);
            fprintf(file, "duration = %d\n\n", p->io_operations[j].duration);
        }
    }

    fclose(file);
    return 1;
}