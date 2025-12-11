<<<<<<< HEAD
#ifndef UTILS_H

#define UTILS_H

#include "../Interface/interface_utils.h"

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256

void scan_config_directory(AppContext *app);
char **get_algorithms(int *count);

=======
/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */

#ifndef UTILS_H

#define UTILS_H

void get_all_files_in_directory(char *dir_path, char files[][20], int *count);

>>>>>>> origin/main
#endif // UTILS_H