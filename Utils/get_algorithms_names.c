/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */

#include <stdio.h>     // for printf and perror
#include <string.h>    // for strcmp, strncpy, strrchr
#include <dirent.h>    // for listing files in a folder
#include <stdlib.h>    // for malloc and free

char **get_algorithms(int *count) {    
    DIR *dir = opendir("Algorithms/");
    if (!dir) return NULL;

    char **list = malloc(sizeof(char*) * 50); // up to 50 algos
    *count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".c") != 0) continue;

        int len = ext - entry->d_name;

        char *name = malloc(len + 1);
        strncpy(name, entry->d_name, len);
        name[len] = '\0';

        list[*count] = name;
        (*count)++;
    }

    closedir(dir);
    return list;
}