#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

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

void update_interface_after_load() {
    // Clear legend
    GtkWidget *child = gtk_widget_get_first_child(legend_box);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(legend_box), child);
        child = next;
    }

    // Rebuild legend
    if (total_processes == 0) {
        gtk_box_append(GTK_BOX(legend_box), gtk_label_new("(No process loaded)"));
    } else {
        for(int i=0; i<total_processes; i++) {
            char buf[100];
            sprintf(buf, "%s : Arr %d | Bur %d | Prio %d", 
                initial_processes[i].id, initial_processes[i].arrival_time, 
                initial_processes[i].duration, initial_processes[i].priority);
            GtkWidget *l = gtk_label_new(buf);
            gtk_widget_set_halign(l, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(legend_box), l);
        }
    }

    // Refresh Drawing
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

    double start_x = 20;
    double scale = 25.0; 
    double axis_y = height - 50;
    int needed_width = start_x + (max_end * scale) + 50;
    if (needed_width > width) gtk_drawing_area_set_content_width(area, needed_width);

    cairo_set_source_rgb(cr, 0.18, 0.20, 0.23); cairo_paint(cr);
    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);

    for (int t = 0; t <= max_end; t++) {
        double x = start_x + (t * scale);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.3); cairo_move_to(cr, x, 30); cairo_line_to(cr, x, axis_y); cairo_stroke(cr);
        cairo_set_source_rgb(cr, 1, 1, 1);
        char num[20]; sprintf(num, "%d", t); cairo_move_to(cr, x - 4, axis_y + 15); cairo_show_text(cr, num);
    }
    cairo_set_source_rgb(cr, 1, 1, 1); cairo_set_line_width(cr, 2);
    cairo_move_to(cr, start_x, axis_y); cairo_line_to(cr, start_x + (max_end * scale), axis_y); cairo_stroke(cr);

    for (int i = 0; i < slice_count; i++) {
        GanttSlice s = slices[i];
        double x = start_x + (s.start * scale);
        double w = s.duration * scale;
        double y = axis_y - 60;
        double h = 40;
        GdkRGBA color; gdk_rgba_parse(&color, s.color);
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1.0);
        // Rounded corners for Gantt blocks
        double r = 5.0; 
        cairo_move_to(cr, x + r, y);
        cairo_line_to(cr, x + w - r, y);
        cairo_curve_to(cr, x + w, y, x + w, y, x + w, y + r);
        cairo_line_to(cr, x + w, y + h - r);
        cairo_curve_to(cr, x + w, y + h, x + w, y + h, x + w - r, y + h);
        cairo_line_to(cr, x + r, y + h);
        cairo_curve_to(cr, x, y + h, x, y + h, x, y + h - r);
        cairo_line_to(cr, x, y + r);
        cairo_curve_to(cr, x, y, x, y, x + r, y);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 1, 1, 1); cairo_set_line_width(cr, 1); cairo_stroke(cr);
        if (w > 20) {
            cairo_set_source_rgb(cr, 0, 0, 0); char buf[20]; sprintf(buf, "%s", s.pid);
            cairo_move_to(cr, x + 5, y + 25); cairo_show_text(cr, buf);
        }
    }
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

// --- BEAUTIFUL CSS ---
static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        // 1. Window & General
        "window { background-color: #282c34; color: #abb2bf; font-family: Sans; font-size: 14px; }"
        "label { color: #dcdfe4; }" 
        
        // 2. Sidebar styling
        ".sidebar { background-color: #21252b; color: #ffffff; padding: 15px; border-right: 1px solid #181a1f; }"
        ".sidebar label { color: #dcdfe4; font-weight: bold; margin-bottom: 5px; }"

        // 3. Modern Flat Blue Buttons
        ".accent-button { background-color: #61afef; color: #ffffff; border-radius: 6px; padding: 8px 12px; border: none; transition: all 0.2s; }"
        ".accent-button label { color: #ffffff; font-weight: bold; }"
        ".accent-button:hover { background-color: #528bff; }"
        ".accent-button:active { background-color: #3d6dc2; transform: translateY(1px); }"

        // 4. Text Input Area
        "textview { border-radius: 6px; border: 1px solid #181a1f; }"
        "textview text { background-color: #ffffff; color: #282c34; }"

        // 5. Dropdown & Popover
        "dropdown { background-color: #ffffff; color: #282c34; border-radius: 6px; padding: 5px; border: 1px solid #abb2bf; }"
        "dropdown label { color: #282c34; }" 
        "dropdown arrow { color: #282c34; }"
        "popover { background-color: #ffffff; color: #282c34; border-radius: 6px; border: 1px solid #abb2bf; }"
        "popover listview { background-color: #ffffff; color: #282c34; }"
        "popover listview row { padding: 8px; }"
        "popover listview row:selected { background-color: #61afef; color: #ffffff; }"
        "popover listview row:selected label { color: #ffffff; }"; 
    
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    load_css();
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Scheduler Simulator Pro");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(main_window), paned);
    gtk_paned_set_position(GTK_PANED(paned), 380);

    // Sidebar
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_add_css_class(sidebar, "sidebar"); 
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);

    // Header
    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("1. CONFIGURATION"));

    // File Selection
    GtkWidget *btn_open = gtk_button_new_with_label("📂 Select Config File...");
    gtk_widget_add_css_class(btn_open, "accent-button");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_config_clicked), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_open);

    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("OR Paste Config Below:"));
    
    // Text Area
    GtkWidget *scrolled_text = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled_text, -1, 150);
    config_text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(config_text_view), GTK_WRAP_WORD);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(config_text_view), 8);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(config_text_view), 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_text), config_text_view);
    gtk_box_append(GTK_BOX(sidebar), scrolled_text);

    // Load Text Button
    GtkWidget *btn_load_text = gtk_button_new_with_label("⬇ Load from Text");
    gtk_widget_add_css_class(btn_load_text, "accent-button");
    g_signal_connect(btn_load_text, "clicked", G_CALLBACK(on_load_text_clicked), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_load_text);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(sep, 10);
    gtk_widget_set_margin_bottom(sep, 10);
    gtk_box_append(GTK_BOX(sidebar), sep);

    // Algo Selection
    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("2. ALGORITHM"));
    const char *algos[] = {"1. FCFS (First Come First Served)", "2. Round Robin", "3. SJF (Shortest Job First)", 
                           "4. Preemptive Priority", "5. Multilevel Static", "6. Multilevel Aging", "7. SRT", NULL};
    GtkWidget *dropdown = gtk_drop_down_new_from_strings(algos);
    g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_algo_changed), NULL);
    gtk_box_append(GTK_BOX(sidebar), dropdown);

    // Quantum Box
    quantum_control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(quantum_control_box, 10);
    gtk_box_append(GTK_BOX(quantum_control_box), gtk_label_new("Quantum (Time Slice):"));
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 3);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_append(GTK_BOX(quantum_control_box), spin);
    gtk_box_append(GTK_BOX(sidebar), quantum_control_box);
    gtk_widget_set_visible(quantum_control_box, FALSE); 

    // Legend
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(sep2, 10);
    gtk_box_append(GTK_BOX(sidebar), sep2);
    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("PROCESS LEGEND"));
    
    legend_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_append(GTK_BOX(sidebar), legend_box);
    GtkWidget *empty_lbl = gtk_label_new("No config loaded.");
    gtk_widget_set_opacity(empty_lbl, 0.5);
    gtk_box_append(GTK_BOX(legend_box), empty_lbl);

    // Draw Area
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, -1, 500); 
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_function, NULL, NULL);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
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
