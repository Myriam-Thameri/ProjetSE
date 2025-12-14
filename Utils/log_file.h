/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#ifndef LOGFILE_H

#define LOGFILE_H


void remove_extension(char *filename);

int init_log(const char *algo_name,  const char *config_file);

void close_log();

int log_print(const char *format, ...);

#endif
