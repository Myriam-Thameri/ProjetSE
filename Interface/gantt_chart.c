#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include "gantt_chart.h"

// Global storage for Gantt chart slices
GanttSlice slices[MAX_SLICES];
int slice_count = 0;

// Predefined color palette for processes
static const char* COLOR_PALETTE[] = {
    "#3498db", "#e74c3c", "#2ecc71", "#f39c12", "#9b59b6",
    "#1abc9c", "#e67e22", "#34495e", "#16a085", "#c0392b",
    "#27ae60", "#2980b9", "#8e44ad", "#d35400", "#7f8c8d"
};
static const int COLOR_COUNT = 15;

#define MIN_PIXELS_PER_TIME_UNIT 30

// Add a time slice to the Gantt chart
void add_gantt_slice(const char* pid, int start, int duration, const char* color) {
    if (slice_count >= MAX_SLICES) {
        g_warning("Maximum Gantt slices reached (%d)", MAX_SLICES);
        return;
    }
    
    // Merge with previous slice if same process and consecutive time
    if (slice_count > 0 && 
        strcmp(slices[slice_count-1].pid, pid) == 0 && 
        (slices[slice_count-1].start + slices[slice_count-1].duration) == start) {
        slices[slice_count-1].duration += duration;
    } else {
        // Add new slice
        strncpy(slices[slice_count].pid, pid, MAX_PID_LEN - 1);
        slices[slice_count].pid[MAX_PID_LEN - 1] = '\0';
        slices[slice_count].start = start;
        slices[slice_count].duration = duration;
        slices[slice_count].color = color ? color : get_process_color(pid);
        slice_count++;
    }
}

// Clear all slices (for new simulation)
void clear_gantt_slices(void) {
    slice_count = 0;
    memset(slices, 0, sizeof(slices));
}

// Get consistent color for a process ID
const char* get_process_color(const char* pid) {
    if (!pid) return COLOR_PALETTE[0];
    
    // Simple hash to get consistent color for same PID
    unsigned int hash = 0;
    for (int i = 0; pid[i] != '\0'; i++) {
        hash = hash * 31 + pid[i];
    }
    
    return COLOR_PALETTE[hash % COLOR_COUNT];
}

// Get the required width for the Gantt chart
static int get_gantt_required_width(void) {
    if (slice_count == 0) return 400; // Largeur par défaut
    
    int total_time = 0;
    for (int i = 0; i < slice_count; i++) {
        int end = slices[i].start + slices[i].duration;
        if (end > total_time) total_time = end;
    }
    
    const int MARGIN = 40;
    int min_chart_width = total_time * MIN_PIXELS_PER_TIME_UNIT;
    
    return min_chart_width + 2 * MARGIN;
}

// Drawing callback for the Gantt chart
static void gantt_draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    int required_width = get_gantt_required_width();
    gtk_widget_set_size_request(GTK_WIDGET(area), required_width, 150);
    if (slice_count == 0) {
        // Draw empty state
        cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, width/2 - 100, height/2);
        cairo_show_text(cr, "No scheduling data available");
        return;
    }
    
    // Calculate total time
    int total_time = 0;
    for (int i = 0; i < slice_count; i++) {
        int end = slices[i].start + slices[i].duration;
        if (end > total_time) total_time = end;
    }
    
    if (total_time == 0) return;
    
    // Layout constants
    const int MARGIN = 40;
    const int BAR_HEIGHT = 50;
    const int LABEL_HEIGHT = 30;
    const int TIME_MARKER_HEIGHT = 25;

    // NOUVEAU: Calculer la largeur nécessaire basée sur la largeur minimale par unité
    int min_chart_width = total_time * MIN_PIXELS_PER_TIME_UNIT;
    int available_width = width - 2 * MARGIN;
    
    int chart_width = (min_chart_width > available_width) ? min_chart_width : available_width;
    
    int chart_top = MARGIN + LABEL_HEIGHT;
    
    // Draw background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, MARGIN, chart_top, chart_width, BAR_HEIGHT);
    cairo_fill(cr);
    
    // Draw border
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 1);
    cairo_rectangle(cr, MARGIN, chart_top, chart_width, BAR_HEIGHT);
    cairo_stroke(cr);
    
    // Draw slices
    for (int i = 0; i < slice_count; i++) {
        GanttSlice *slice = &slices[i];
        
        double x = MARGIN + (double)slice->start / total_time * chart_width;
        double w = (double)slice->duration / total_time * chart_width;
        
        // Parse color (simple hex color parsing)
        int r, g, b;
        if (sscanf(slice->color, "#%02x%02x%02x", &r, &g, &b) == 3) {
            cairo_set_source_rgb(cr, r/255.0, g/255.0, b/255.0);
        } else {
            cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);
        }
        
        // Draw slice rectangle
        cairo_rectangle(cr, x, chart_top, w, BAR_HEIGHT);
        cairo_fill_preserve(cr);
        
        // Draw slice border
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_set_line_width(cr, 1);
        cairo_stroke(cr);
        
        // Draw process ID if space allows
        if (w > 30) {
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 12);
            
            cairo_text_extents_t extents;
            cairo_text_extents(cr, slice->pid, &extents);
            
            cairo_move_to(cr, x + w/2 - extents.width/2, 
                         chart_top + BAR_HEIGHT/2 + extents.height/2);
            cairo_show_text(cr, slice->pid);
        }
    }
    
    // Draw ALL time markers (every single time unit)
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 9);
    
    // Draw a marker for every single time unit from 0 to total_time
    for (int t = 0; t <= total_time; t++) {
        double x = MARGIN + (double)t / total_time * chart_width;
        
        // Draw tick line
        cairo_set_line_width(cr, 1);
        cairo_move_to(cr, x, chart_top + BAR_HEIGHT);
        cairo_line_to(cr, x, chart_top + BAR_HEIGHT + 5);
        cairo_stroke(cr);
        
        // Draw time label
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%d", t);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, time_str, &extents);
        
        // Center the text below the tick
        cairo_move_to(cr, x - extents.width/2, 
                     chart_top + BAR_HEIGHT + TIME_MARKER_HEIGHT);
        cairo_show_text(cr, time_str);
    }
    
    // Draw title
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 14);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr, MARGIN, MARGIN - 10);
    cairo_show_text(cr, "Gantt Chart - Process Execution Timeline");
}

// Create the Gantt chart widget
GtkWidget* create_gantt_chart_widget(void) {
    GtkWidget *drawing_area = gtk_drawing_area_new();
    //gtk_widget_set_size_request(drawing_area, -1, 150);
    //gtk_widget_set_hexpand(drawing_area, TRUE); 
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), gantt_draw_function, NULL, NULL);
    
    return drawing_area;
}