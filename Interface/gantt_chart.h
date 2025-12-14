/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université de Tunis El Manar
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#ifndef GANTT_CHART_H
#define GANTT_CHART_H

#include <gtk/gtk.h>

#define MAX_SLICES 1000
#define MAX_PID_LEN 32

typedef struct {
    char pid[MAX_PID_LEN];
    int start;
    int duration;
    const char* color;
} GanttSlice;

typedef struct {
    char pid[MAX_PID_LEN];
    int start;
    int duration;
    const char* color;
    char io_type[32];  
} IOSlice;

extern GanttSlice slices[MAX_SLICES];
extern int slice_count;

extern IOSlice io_slices[MAX_SLICES];
extern int io_slice_count;

void add_gantt_slice(const char* pid, int start, int duration, const char* color);
void clear_gantt_slices(void);

void add_io_slice(const char* pid, int start, int duration, const char* color, const char* io_type);
void clear_io_slices(void);

const char* get_process_color(const char* pid);

GtkWidget* create_gantt_chart_widget(void);

#endif 