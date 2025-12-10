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

void trim(char* str);// function to remove spaces, tabulation, \n   from the start and end of a string to get a clean format 

int load_config( char* filename, Config* cfg); // loads the config from the file and extract the processes and their attributes from the text file

#endif // CONFIG_H
