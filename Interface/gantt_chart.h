/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */

#ifndef GANTT_CHART_H
#define GANTT_CHART_H

#define MAX_SLICES 1000
#define MAX_PID_LEN 32
#include <gtk/gtk.h>

// Structure to represent a time slice in the Gantt chart
typedef struct {
    char pid[MAX_PID_LEN];
    int start;
    int duration;
    const char* color;
} GanttSlice;

// Global slice storage
extern GanttSlice slices[MAX_SLICES];
extern int slice_count;

// Function to add a slice (merges consecutive slices of same process)
void add_gantt_slice(const char* pid, int start, int duration, const char* color);

// Function to clear all slices (call before starting a new scheduling simulation)
void clear_gantt_slices(void);

// Function to get a color for a process ID (consistent coloring)
const char* get_process_color(const char* pid);

// Function to render the Gantt chart in GTK
GtkWidget* create_gantt_chart_widget(void);

#endif // GANTT_CHART_H