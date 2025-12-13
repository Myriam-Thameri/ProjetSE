#ifndef GANTT_CHART_H
#define GANTT_CHART_H

#include <gtk/gtk.h>

#define MAX_SLICES 1000
#define MAX_PID_LEN 32

// Structure for a single time slice in the Gantt chart
typedef struct {
    char pid[MAX_PID_LEN];
    int start;
    int duration;
    const char* color;
} GanttSlice;

// Structure for I/O operations
typedef struct {
    char pid[MAX_PID_LEN];
    int start;
    int duration;
    const char* color;
    char io_type[32];  // Optional: "disk", "network", etc.
} IOSlice;

// Global storage
extern GanttSlice slices[MAX_SLICES];
extern int slice_count;

extern IOSlice io_slices[MAX_SLICES];
extern int io_slice_count;

// Process Gantt functions
void add_gantt_slice(const char* pid, int start, int duration, const char* color);
void clear_gantt_slices(void);

// I/O Gantt functions
void add_io_slice(const char* pid, int start, int duration, const char* color, const char* io_type);
void clear_io_slices(void);

// Utility functions
const char* get_process_color(const char* pid);

// Widget creation
GtkWidget* create_gantt_chart_widget(void);

#endif // GANTT_CHART_H