#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

// Suppress deprecation warnings
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// --- Data Structures ---

typedef struct {
    int start_time;
    int duration;
} VisIO;

typedef struct {
    char id[10];
    int arrival_time;
    int duration;       
    int remaining_time; 
    int executed_time;  
    int priority;       
    int wait_time;      
    const char *color;
    VisIO ios[20];      
    int io_count;       
    int current_io_idx; 
    int io_remaining;   
    int in_io;          
    int finished;       
} VisProcess;

typedef struct {
    char pid[10];
    double start;
    double duration;
    const char *color;
} GanttSlice;

// --- Global Data ---

#define MAX_SLICES 5000
GanttSlice slices[MAX_SLICES];
int slice_count = 0;

VisProcess initial_processes[20];
int total_processes = 0;

// Pastel Color Palette for Processes
const char* COLORS[] = {
    "#E06C75", "#98C379", "#61AFEF", "#E5C07B", "#C678DD", "#56B6C2", "#D19A66", 
    "#F44336", "#2196F3", "#4CAF50", "#FFEB3B", "#9C27B0"
};

static int current_algo_index = 0;
static int current_quantum = 3;

// Global Widgets
static GtkWidget *drawing_area; 
static GtkWidget *quantum_control_box; 
static GtkWidget *legend_box; 
static GtkWidget *main_window; 
static GtkWidget *config_text_view;

// --- HELPER FUNCTIONS ---

void trim(char* s) {
    char *start = s;
    while(*start && isspace((unsigned char)*start)) start++;
    memmove(s, start, strlen(start)+1);
    char* end = s + strlen(s) - 1;
    while(end > s && isspace((unsigned char)*end)) *end-- = '\0';
}

// Fonction pour dessiner le carré de couleur dans la légende
static void draw_color_square(GtkDrawingArea *area, cairo_t *cr, int w, int h, gpointer data) {
    const char *color = (const char*)data;
    GdkRGBA rgba;
    gdk_rgba_parse(&rgba, color);
    cairo_set_source_rgb(cr, rgba.red, rgba.green, rgba.blue);
    
    // Rectangle avec coins arrondis
    double x = 0, y = 0, width = 20, height = 20, radius = 4;
    cairo_move_to(cr, x + radius, y);
    cairo_line_to(cr, x + width - radius, y);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI/2, 0);
    cairo_line_to(cr, x + width, y + height - radius);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI/2);
    cairo_line_to(cr, x + radius, y + height);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI/2, M_PI);
    cairo_line_to(cr, x, y + radius);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_fill(cr);
}

void update_interface_after_load() {
    GtkWidget *child = gtk_widget_get_first_child(legend_box);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(legend_box), child);
        child = next;
    }

    if (total_processes == 0) {
        GtkWidget *empty_label = gtk_label_new("⚠️ No config loaded");
        gtk_widget_set_opacity(empty_label, 0.6);
        gtk_box_append(GTK_BOX(legend_box), empty_label);
    } else {
        for(int i=0; i<total_processes; i++) {
            // Créer une box horizontale pour chaque processus
            GtkWidget *process_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_widget_add_css_class(process_box, "legend-item");
            
            // Carré de couleur
            GtkWidget *color_box = gtk_drawing_area_new();
            gtk_widget_set_size_request(color_box, 20, 20);
            gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(color_box), 
                draw_color_square,
                (gpointer)initial_processes[i].color, 
                NULL);
            
            gtk_box_append(GTK_BOX(process_box), color_box);
            
            // Texte du processus
            char buf[100];
            sprintf(buf, "<b>%s</b> · Arr: %d | Dur: %d | Prio: %d", 
                initial_processes[i].id, initial_processes[i].arrival_time, 
                initial_processes[i].duration, initial_processes[i].priority);
            GtkWidget *l = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(l), buf);
            gtk_widget_set_halign(l, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(process_box), l);
            
            gtk_box_append(GTK_BOX(legend_box), process_box);
        }
    }

    if (drawing_area) gtk_widget_queue_draw(drawing_area);
}

// --- PARSING LOGIC ---

void parse_config_content(char* content) {
    total_processes = 0;
    memset(initial_processes, 0, sizeof(initial_processes));
    int current_proc_idx = -1;
    int current_io_idx = -1;

    char *line = strtok(content, "\n");
    while (line != NULL) {
        trim(line);
        if (line[0] == '#' || strlen(line) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }

        if (line[0] == '[') {
            if (strstr(line, "process") && !strstr(line, "process_io")) {
                if (total_processes < 20) {
                    total_processes++;
                    current_proc_idx = total_processes - 1;
                    current_io_idx = -1;
                    VisProcess *p = &initial_processes[current_proc_idx];
                    strcpy(p->id, "UNK");
                    p->color = COLORS[current_proc_idx % 12];
                }
            }
            else if (strstr(line, "process_io")) {
                if (current_proc_idx >= 0) current_io_idx++; 
            }
        } 
        else {
            char* eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char* key = line;
                char* value = eq + 1;
                trim(key); trim(value);

                if (current_proc_idx >= 0) {
                    VisProcess *p = &initial_processes[current_proc_idx];
                    if (strcmp(key, "ID") == 0) strcpy(p->id, value);
                    else if (strcmp(key, "arrival_time") == 0) p->arrival_time = atoi(value);
                    else if (strcmp(key, "execution_time") == 0) p->duration = atoi(value);
                    else if (strcmp(key, "priority") == 0) p->priority = atoi(value);
                    else if (strcmp(key, "io_count") == 0) p->io_count = atoi(value);
                    
                    if (current_io_idx >= 0 && current_io_idx < 20) {
                         if (strcmp(key, "start_time") == 0) p->ios[current_io_idx].start_time = atoi(value);
                         else if (strcmp(key, "duration") == 0) p->ios[current_io_idx].duration = atoi(value);
                    }
                }
            }
        }
        line = strtok(NULL, "\n");
    }
    update_interface_after_load();
}

static void on_load_text_clicked(GtkWidget *btn, gpointer data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(config_text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    if (text && strlen(text) > 0) {
        parse_config_content(text);
        g_free(text);
    }
}

static void on_file_open_response(GtkNativeDialog *native, int response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        GFile *file = gtk_file_chooser_get_file(chooser);
        char *path = g_file_get_path(file);
        
        char *content = NULL;
        gsize length = 0;
        GError *error = NULL;
        
        if (g_file_get_contents(path, &content, &length, &error)) {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(config_text_view));
            gtk_text_buffer_set_text(buffer, content, -1);
            parse_config_content(content);
            g_free(content);
        } else {
            g_print("Error: %s\n", error->message);
            g_error_free(error);
        }
        g_free(path);
        g_object_unref(file);
    }
    g_object_unref(native);
}

static void on_open_config_clicked(GtkWidget *widget, gpointer data) {
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Select Config File",
                                                               GTK_WINDOW(main_window),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                                               "_Open",
                                                               "_Cancel");
    g_signal_connect(native, "response", G_CALLBACK(on_file_open_response), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

// --- SIMULATION ENGINE ---

void add_slice(const char* pid, int start, int duration, const char* color) {
    if (slice_count >= MAX_SLICES) return;
    if (slice_count > 0 && strcmp(slices[slice_count-1].pid, pid) == 0 && 
        (slices[slice_count-1].start + slices[slice_count-1].duration) == start) {
        slices[slice_count-1].duration += duration;
    } else {
        strcpy(slices[slice_count].pid, pid);
        slices[slice_count].start = start;
        slices[slice_count].duration = duration;
        slices[slice_count].color = color;
        slice_count++;
    }
}

void handle_io_background(VisProcess procs[], int count) {
    for (int i = 0; i < count; i++) {
        if (procs[i].in_io) {
            procs[i].io_remaining--;
            if (procs[i].io_remaining <= 0) {
                procs[i].in_io = 0;
                procs[i].current_io_idx++;
            }
        }
    }
}

int check_start_io(VisProcess *p) {
    if (p->current_io_idx < p->io_count) {
        if (p->executed_time == p->ios[p->current_io_idx].start_time) {
            p->in_io = 1;
            p->io_remaining = p->ios[p->current_io_idx].duration;
            return 1; 
        }
    }
    return 0; 
}

// --- ALGORITHMS ---

void run_fcfs(VisProcess procs[], int count) {
    int current_time = 0, completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        int idx = -1, min_arr = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].arrival_time < min_arr) { min_arr = procs[i].arrival_time; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++; check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        } 
        current_time++;
    }
}

void run_rr(VisProcess procs[], int count) {
    int current_time = 0, completed = 0, quantum = (current_quantum < 1) ? 1 : current_quantum;
    int queue[2000], q_start = 0, q_end = 0, in_queue[100] = {0}; 
    for(int i=0; i<count; i++) if (procs[i].arrival_time == 0) { queue[q_end++] = i; in_queue[i] = 1; }
    int current_proc_idx = -1, quantum_counter = 0;

    while (completed < count) {
        for (int i = 0; i < count; i++) {
            if (procs[i].in_io) {
                procs[i].io_remaining--;
                if (procs[i].io_remaining <= 0) {
                    procs[i].in_io = 0; procs[i].current_io_idx++;
                    if (!procs[i].finished) { queue[q_end++] = i; in_queue[i] = 1; }
                }
            }
        }
        for (int i = 0; i < count; i++) {
            if (!in_queue[i] && !procs[i].finished && !procs[i].in_io && procs[i].arrival_time == current_time) {
                queue[q_end++] = i; in_queue[i] = 1;
            }
        }
        if (current_proc_idx == -1 && q_start < q_end) {
            current_proc_idx = queue[q_start++]; in_queue[current_proc_idx] = 0; quantum_counter = 0;
        }
        if (current_proc_idx != -1) {
            VisProcess *p = &procs[current_proc_idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++; quantum_counter++;
            int io_started = check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; current_proc_idx = -1; }
            else if (io_started) { current_proc_idx = -1; }
            else if (quantum_counter >= quantum) {
                queue[q_end++] = current_proc_idx; in_queue[current_proc_idx] = 1; current_proc_idx = -1;
            }
        }
        current_time++;
    }
}

void run_sjf(VisProcess procs[], int count) {
    int current_time = 0, completed = 0, current_proc_idx = -1;
    while (completed < count) {
        handle_io_background(procs, count);
        if (current_proc_idx == -1) {
            int idx = -1, min_dur = INT_MAX;
            for (int i = 0; i < count; i++) {
                if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                    if (procs[i].duration < min_dur) { min_dur = procs[i].duration; idx = i; }
                }
            }
            current_proc_idx = idx;
        }
        if (current_proc_idx != -1) {
            VisProcess *p = &procs[current_proc_idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++;
            if (check_start_io(p)) current_proc_idx = -1;
            else if (p->remaining_time == 0) { p->finished = 1; completed++; current_proc_idx = -1; }
        }
        current_time++;
    }
}

void run_priority(VisProcess procs[], int count) {
    int current_time = 0, completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        int idx = -1, best_prio = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].priority < best_prio) { best_prio = procs[i].priority; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++; check_start_io(p); 
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

void run_multilevel_static(VisProcess procs[], int count) { run_priority(procs, count); }

void run_multilevel_aging(VisProcess procs[], int count) {
    int current_time = 0, completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        for(int i=0; i<count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                procs[i].wait_time++;
                if(procs[i].wait_time >= 5 && procs[i].priority > 1) { procs[i].priority--; procs[i].wait_time = 0; }
            }
        }
        int idx = -1, best_prio = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].priority < best_prio) { best_prio = procs[i].priority; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            p->wait_time = 0; add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++; check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

void run_srt(VisProcess procs[], int count) {
    int current_time = 0, completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        int idx = -1, min_rem = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].remaining_time < min_rem) { min_rem = procs[i].remaining_time; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--; p->executed_time++; check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

void simulate_current_algo() {
    slice_count = 0;
    if (total_processes == 0) return;
    VisProcess working_procs[20]; 
    for(int i=0; i<total_processes; i++) {
        working_procs[i] = initial_processes[i];
        working_procs[i].remaining_time = working_procs[i].duration;
        working_procs[i].executed_time = 0;
        working_procs[i].finished = 0;
        working_procs[i].in_io = 0;
        working_procs[i].current_io_idx = 0;
        working_procs[i].wait_time = 0;
        working_procs[i].io_remaining = 0;
    }
    switch(current_algo_index) {
        case 0: run_fcfs(working_procs, total_processes); break;
        case 1: run_rr(working_procs, total_processes); break;
        case 2: run_sjf(working_procs, total_processes); break;
        case 3: run_priority(working_procs, total_processes); break;
        case 4: run_multilevel_static(working_procs, total_processes); break;
        case 5: run_multilevel_aging(working_procs, total_processes); break;
        case 6: run_srt(working_procs, total_processes); break;
    }
}

// --- Drawing ---

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    simulate_current_algo();
    double max_end = 0;
    for(int i=0; i<slice_count; i++) 
        if((slices[i].start + slices[i].duration) > max_end) 
            max_end = slices[i].start + slices[i].duration;
    max_end += 2;

    double start_x = 40;
    double scale = 35.0;
    double axis_y = height / 2 + 80;
    
    // ⚠️ FORCEZ UNE GRANDE LARGEUR pour que le scroll apparaisse
    int needed_width = start_x + (max_end * scale) + 100;
    if (needed_width < 1500) needed_width = 1500;  // Changé de 800 à 1500
    
    gtk_drawing_area_set_content_width(area, needed_width);
    gtk_drawing_area_set_content_height(area, 600);  // Hauteur fixe aussi

    // Fond dégradé
    cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(gradient, 0, 0.16, 0.18, 0.20);
    cairo_pattern_add_color_stop_rgb(gradient, 1, 0.18, 0.20, 0.23);
    cairo_set_source(cr, gradient);
    cairo_paint(cr);
    cairo_pattern_destroy(gradient);

    cairo_set_line_width(cr, 1.5);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);

    // Titre du diagramme
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, start_x, 40);
    cairo_show_text(cr, "Gantt Chart - Process Scheduling");
    cairo_set_font_size(cr, 12);

    // Grille verticale avec style amélioré
    for (int t = 0; t <= max_end; t++) {
        double x = start_x + (t * scale);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
        cairo_set_line_width(cr, 1);
        cairo_move_to(cr, x, 80);
        cairo_line_to(cr, x, axis_y);
        cairo_stroke(cr);
        
        // Numéros avec ombre
        cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
        char num[20];
        sprintf(num, "%d", t);
        cairo_move_to(cr, x - 5, axis_y + 25);
        cairo_show_text(cr, num);
        
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, x - 6, axis_y + 23);
        cairo_show_text(cr, num);
    }

    // Axe principal avec style
    cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
    cairo_set_line_width(cr, 3);
    cairo_move_to(cr, start_x, axis_y);
    cairo_line_to(cr, start_x + (max_end * scale), axis_y);
    cairo_stroke(cr);

    // Blocs Gantt avec ombres et coins arrondis - PLUS GRANDS
    for (int i = 0; i < slice_count; i++) {
        GanttSlice s = slices[i];
        double x = start_x + (s.start * scale);
        double w = s.duration * scale;
        double y = axis_y - 100;  // Plus haut
        double h = 70;  // Plus grand
        double radius = 10;
        
        GdkRGBA color;
        gdk_rgba_parse(&color, s.color);
        
        // Ombre portée
        cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
        cairo_move_to(cr, x + radius, y + 4);
        cairo_line_to(cr, x + w - radius, y + 4);
        cairo_arc(cr, x + w - radius, y + radius + 4, radius, -M_PI/2, 0);
        cairo_line_to(cr, x + w, y + h - radius + 4);
        cairo_arc(cr, x + w - radius, y + h - radius + 4, radius, 0, M_PI/2);
        cairo_line_to(cr, x + radius, y + h + 4);
        cairo_arc(cr, x + radius, y + h - radius + 4, radius, M_PI/2, M_PI);
        cairo_line_to(cr, x, y + radius + 4);
        cairo_arc(cr, x + radius, y + radius + 4, radius, M_PI, 3*M_PI/2);
        cairo_fill(cr);
        
        // Bloc principal avec dégradé
        cairo_pattern_t *block_gradient = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgba(block_gradient, 0, color.red, color.green, color.blue, 1.0);
        cairo_pattern_add_color_stop_rgba(block_gradient, 1, color.red * 0.7, color.green * 0.7, color.blue * 0.7, 1.0);
        cairo_set_source(cr, block_gradient);
        
        cairo_move_to(cr, x + radius, y);
        cairo_line_to(cr, x + w - radius, y);
        cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI/2, 0);
        cairo_line_to(cr, x + w, y + h - radius);
        cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI/2);
        cairo_line_to(cr, x + radius, y + h);
        cairo_arc(cr, x + radius, y + h - radius, radius, M_PI/2, M_PI);
        cairo_line_to(cr, x, y + radius);
        cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(block_gradient);
        
        // Bordure brillante
        cairo_set_source_rgba(cr, 1, 1, 1, 0.4);
        cairo_set_line_width(cr, 2.5);
        cairo_stroke(cr);
        
        // Texte avec ombre - PLUS GRAND
        if (w > 20) {
            char buf[20];
            sprintf(buf, "%s", s.pid);
            
            cairo_set_font_size(cr, 14);
            cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
            cairo_move_to(cr, x + w/2 - 12, y + h/2 + 7);
            cairo_show_text(cr, buf);
            
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_move_to(cr, x + w/2 - 13, y + h/2 + 6);
            cairo_show_text(cr, buf);
            cairo_set_font_size(cr, 12);
        }
    }
    
    // Légende "Time" en bas
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_font_size(cr, 13);
    cairo_move_to(cr, start_x + (max_end * scale) / 2 - 20, axis_y + 50);

}

static void on_quantum_changed(GtkSpinButton *spin, gpointer data) {
    current_quantum = gtk_spin_button_get_value_as_int(spin);
    if (drawing_area) gtk_widget_queue_draw(drawing_area);
}

static void on_algo_changed(GObject *object, GParamSpec *pspec, gpointer data) {
    GtkDropDown *dropdown = GTK_DROP_DOWN(object);
    current_algo_index = gtk_drop_down_get_selected(dropdown);
    if (current_algo_index == 1) gtk_widget_set_visible(quantum_control_box, TRUE);
    else gtk_widget_set_visible(quantum_control_box, FALSE);
    if (drawing_area) gtk_widget_queue_draw(drawing_area);
}

// --- CSS AMÉLIORÉ COMPLET ---
static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        // 1. Window & General
        "window { background-color: #282c34; color: #abb2bf; font-family: 'Segoe UI', Sans; font-size: 14px; }"
        "label { color: #dcdfe4; }" 
        
        // 2. Sidebar styling
        ".sidebar { background-color: #21252b; color: #ffffff; padding: 15px; border-right: 1px solid #181a1f; }"
        ".sidebar label { color: #dcdfe4; font-weight: bold; margin-bottom: 5px; }"

        // 3. Modern Flat Blue Buttons
        ".accent-button { background: linear-gradient(135deg, #61afef 0%, #528bff 100%); color: #ffffff; border-radius: 8px; padding: 10px 16px; border: none; box-shadow: 0 2px 8px rgba(97, 175, 239, 0.3); transition: all 0.3s ease; }"
        ".accent-button label { color: #ffffff; font-weight: 600; font-size: 14px; }"
        ".accent-button:hover { background: linear-gradient(135deg, #528bff 0%, #4a7fd6 100%); box-shadow: 0 4px 12px rgba(97, 175, 239, 0.5); transform: translateY(-2px); }"
        ".accent-button:active { background: linear-gradient(135deg, #3d6dc2 0%, #2d5ba8 100%); transform: translateY(0px); box-shadow: 0 2px 6px rgba(97, 175, 239, 0.3); }"

        // 4. Text Input Area
        "textview { border-radius: 8px; border: 2px solid #3d4148; background-color: #ffffff; box-shadow: inset 0 1px 3px rgba(0,0,0,0.1); }"
        "textview text { background-color: #ffffff; color: #282c34; padding: 8px; }"
        "textview:focus { border-color: #61afef; }"

        // 5. Dropdown
        "dropdown { background: linear-gradient(180deg, #ffffff 0%, #f8f9fa 100%); color: #282c34; border-radius: 8px; border: 2px solid #d1d5db; padding: 2px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
        "dropdown button { background-color: transparent; color: #282c34; border: none; padding: 8px 12px; }"
        "dropdown button label { color: #282c34; font-weight: 500; }" 
        "dropdown label { color: #282c34; }"
        "dropdown:hover { border-color: #61afef; box-shadow: 0 2px 8px rgba(97, 175, 239, 0.2); }"
        
        // 6. Popover
        "popover.menu { background-color: #ffffff; border-radius: 8px; border: 1px solid #d1d5db; padding: 6px; box-shadow: 0 8px 16px rgba(0,0,0,0.15); }"
        "popover.menu contents { background-color: #ffffff; }"
        "popover.menu listview { background-color: #ffffff; }"
        "popover.menu listview row { padding: 10px 14px; background-color: #ffffff; border-radius: 6px; margin: 2px; }"
        "popover.menu listview row label { color: #282c34; font-size: 13px; }"
        "popover.menu listview row:hover { background-color: #e8f4fd; }"
        "popover.menu listview row:hover label { color: #1e3a5f; }"
        "popover.menu listview row:selected { background: linear-gradient(135deg, #61afef 0%, #528bff 100%); box-shadow: 0 2px 4px rgba(97, 175, 239, 0.3); }"
        "popover.menu listview row:selected label { color: #ffffff; font-weight: 600; }"
        "popover.menu listview row:selected:hover { background: linear-gradient(135deg, #528bff 0%, #4a7fd6 100%); }"
        "popover.menu listview row image { color: #282c34; }"
        "popover.menu listview row:selected image { color: #ffffff; }"
        
        // 7. SpinButton
        "spinbutton { background: linear-gradient(180deg, #ffffff 0%, #f8f9fa 100%); border-radius: 8px; border: 2px solid #d1d5db; box-shadow: 0 2px 4px rgba(0,0,0,0.05); }"
        "spinbutton:focus-within { border-color: #61afef; box-shadow: 0 0 0 3px rgba(97, 175, 239, 0.1); }"
        "spinbutton entry { background-color: transparent; color: #282c34; border: none; font-weight: 600; }"
        "spinbutton button { background-color: transparent; border: none; color: #61afef; }"
        "spinbutton button:hover { background-color: rgba(97, 175, 239, 0.1); }"
        
        // 8. Separator
        "separator { background-color: #3d4148; min-height: 2px; margin-top: 8px; margin-bottom: 8px; opacity: 0.3; }"
        
        // 9. Legend Items
        ".legend-item { color: #dcdfe4; padding: 6px; border-radius: 6px; background-color: rgba(255, 255, 255, 0.05); margin: 3px 0; }"
        ".legend-item:hover { background-color: rgba(97, 175, 239, 0.1); }"
        
        // 10. ScrolledWindow
        "scrolledwindow { border-radius: 8px; }";
    
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), 
                                                GTK_STYLE_PROVIDER(provider), 
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    load_css();
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 750);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(main_window), paned);
    gtk_paned_set_position(GTK_PANED(paned), 380);

    // Sidebar
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(sidebar, 12);
    gtk_widget_set_margin_end(sidebar, 12);
    gtk_widget_set_margin_top(sidebar, 12);
    gtk_widget_set_margin_bottom(sidebar, 12);
    gtk_widget_add_css_class(sidebar, "sidebar");
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);

    // Header
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<span size='large' weight='bold'>⚙️ CONFIGURATION</span>");
    gtk_box_append(GTK_BOX(sidebar), header);

    // File Button
    GtkWidget *btn_open = gtk_button_new_with_label("📂 Select Config File");
    gtk_widget_add_css_class(btn_open, "accent-button");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_config_clicked), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_open);

    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("OR Paste Configuration:"));
    
    // Text Area
    GtkWidget *scrolled_text = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled_text, -1, 140);
    config_text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(config_text_view), GTK_WRAP_WORD);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(config_text_view), 8);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(config_text_view), 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_text), config_text_view);
    gtk_box_append(GTK_BOX(sidebar), scrolled_text);

    // Load Button
    GtkWidget *btn_load_text = gtk_button_new_with_label("⬇️ Load from Text");
    gtk_widget_add_css_class(btn_load_text, "accent-button");
    g_signal_connect(btn_load_text, "clicked", G_CALLBACK(on_load_text_clicked), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_load_text);

    // Separator
    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Algorithm Section
    GtkWidget *algo_header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(algo_header), "<span size='large' weight='bold'>ALGORITHM</span>");
    gtk_box_append(GTK_BOX(sidebar), algo_header);
    
    const char *algos[] = {
        "1️⃣ FCFS (First Come First Served)", 
        "2️⃣ Round Robin", 
        "3️⃣ SJF (Shortest Job First)", 
        "4️⃣ Preemptive Priority", 
        "5️⃣ Multilevel Static", 
        "6️⃣ Multilevel Aging", 
        "7️⃣ SRT (Shortest Remaining Time)", 
        NULL
    };
    GtkWidget *dropdown = gtk_drop_down_new_from_strings(algos);
    g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_algo_changed), NULL);
    gtk_box_append(GTK_BOX(sidebar), dropdown);

    // Quantum Control
    quantum_control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(quantum_control_box, 8);
    GtkWidget *quantum_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(quantum_label), "<b>⏱️ Time Quantum:</b>");
    gtk_widget_set_halign(quantum_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(quantum_control_box), quantum_label);
    
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 3);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_append(GTK_BOX(quantum_control_box), spin);
    gtk_box_append(GTK_BOX(sidebar), quantum_control_box);
    gtk_widget_set_visible(quantum_control_box, FALSE);

    // Legend Section
    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *legend_header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(legend_header), "<span size='large' weight='bold'>📊 PROCESS LEGEND</span>");
    gtk_box_append(GTK_BOX(sidebar), legend_header);
    
    GtkWidget *legend_scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(legend_scrolled),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(legend_scrolled, -1, 200);
    gtk_widget_set_vexpand(legend_scrolled, TRUE);
    
    legend_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(legend_scrolled), legend_box);
    gtk_box_append(GTK_BOX(sidebar), legend_scrolled);
    
    GtkWidget *empty_lbl = gtk_label_new("⚠️ No configuration loaded");
    gtk_widget_set_opacity(empty_lbl, 0.6);
    gtk_box_append(GTK_BOX(legend_box), empty_lbl);

    // Drawing Area avec scrollbars
    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), 1500);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), 600);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_function, NULL, NULL);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                   GTK_POLICY_ALWAYS,    // Force ALWAYS au lieu de AUTOMATIC
                                   GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), drawing_area);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_paned_set_end_child(GTK_PANED(paned), scrolled_window);

    gtk_window_present(GTK_WINDOW(main_window));
}
int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.projetse.final", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
