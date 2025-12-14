/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#ifndef CONFIG_H

#define CONFIG_H

#include "types.h"

typedef struct 
{
    PROCESS processes[20];
    int process_count;
} Config;

void trim(char* str);

int load_config( char* filename, Config* cfg); 
int save_config( char* filename, Config* cfg);
void free_config(Config *cfg);
#endif
