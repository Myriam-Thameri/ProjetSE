#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// --- Structures de Données (Adaptées pour la visualisation) ---

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
    char pid[10]; // ID du processus (P01, P02...)
    double start;
    double duration;
    const char *color;
} GanttSlice;

// --- Données Globales ---

#define MAX_SLICES 5000
GanttSlice slices[MAX_SLICES];
int slice_count = 0;

// Tableau dynamique (Max 20 processus comme dans votre config.h)
VisProcess initial_processes[20];
int total_processes = 0;

// Couleurs prédéfinies pour les processus
const char* COLORS[] = {
    "#E06C75", "#98C379", "#61AFEF", "#E5C07B", "#C678DD", "#56B6C2", "#D19A66", 
    "#F44336", "#2196F3", "#4CAF50", "#FFEB3B", "#9C27B0"
};

static int current_algo_index = 0;
static int current_quantum = 3; // Valeur par défaut, sera écrasée par l'UI

static GtkWidget *drawing_area; 
static GtkWidget *quantum_control_box; 

// --- PARSER DE CONFIGURATION (Adapté de votre Config/config.c) ---

void trim(char* s) {
    char *start = s;
    while(*start && isspace((unsigned char)*start)) start++;
    memmove(s, start, strlen(start)+1);
    char* end = s + strlen(s) - 1;
    while(end > s && isspace((unsigned char)*end)) *end-- = '\0';
}

void load_config_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Erreur: Impossible d'ouvrir %s. Utilisation de données par défaut.\n", path);
        return;
    }

    char line[256];
    int current_proc_idx = -1;
    int current_io_idx = -1;
    total_processes = 0;

    while(fgets(line, sizeof(line), file)) {
        trim(line);
        if (line[0] == '#' || strlen(line) == 0) continue;

        if (line[0] == '[') {
            // Détection de section
            if (strstr(line, "process") && !strstr(line, "process_io")) {
                // Nouveau Processus
                total_processes++;
                current_proc_idx = total_processes - 1;
                current_io_idx = -1;
                
                // Init par défaut
                VisProcess *p = &initial_processes[current_proc_idx];
                strcpy(p->id, "UNK");
                p->arrival_time = 0;
                p->duration = 0;
                p->priority = 0;
                p->io_count = 0;
                p->color = COLORS[current_proc_idx % 12]; // Assigner une couleur
            }
            else if (strstr(line, "process_io")) {
                // Nouvelle IO pour le processus en cours
                if (current_proc_idx >= 0) {
                    VisProcess *p = &initial_processes[current_proc_idx];
                    // On ne se fie pas au "io_count" du fichier pour l'index, on incrémente
                    // (Votre parser original utilise process%d_io%d mais ici on simplifie la lecture séquentielle)
                    // On suppose que les blocs IO suivent le bloc process
                    current_io_idx++; 
                    // p->io_count sera mis à jour ou lu
                }
            }
            continue;
        }

        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* key = line;
            char* value = eq + 1;
            trim(key); trim(value);

            if (current_proc_idx >= 0) {
                VisProcess *p = &initial_processes[current_proc_idx];
                
                // Attributs Processus
                if (strcmp(key, "ID") == 0) strcpy(p->id, value);
                else if (strcmp(key, "arrival_time") == 0) p->arrival_time = atoi(value);
                else if (strcmp(key, "execution_time") == 0) p->duration = atoi(value);
                else if (strcmp(key, "priority") == 0) p->priority = atoi(value);
                else if (strcmp(key, "io_count") == 0) p->io_count = atoi(value); // On stocke, mais on remplit ios[] dynamiquement
                
                // Attributs IO
                if (current_io_idx >= 0 && current_io_idx < 20) {
                     if (strcmp(key, "start_time") == 0) p->ios[current_io_idx].start_time = atoi(value);
                     else if (strcmp(key, "duration") == 0) p->ios[current_io_idx].duration = atoi(value);
                }
            }
        }
    }
    fclose(file);
    printf("Config chargée: %d processus trouvés.\n", total_processes);
}

// --- Moteur de Simulation ---

void add_slice(const char* pid, int start, int duration, const char* color) {
    if (slice_count >= MAX_SLICES) return;
    
    // Fusion visuelle
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
    // On vérifie par rapport au io_count lu dans le fichier
    if (p->current_io_idx < p->io_count) {
        if (p->executed_time == p->ios[p->current_io_idx].start_time) {
            p->in_io = 1;
            p->io_remaining = p->ios[p->current_io_idx].duration;
            return 1; 
        }
    }
    return 0; 
}

// --- ALGORITHMES ---

// 1. FCFS
void run_fcfs(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;

    while (completed < count) {
        handle_io_background(procs, count);

        int idx = -1;
        int min_arrival = INT_MAX;

        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].arrival_time < min_arrival) {
                    min_arrival = procs[i].arrival_time;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        } 
        current_time++;
    }
}

// 2. Round Robin
void run_rr(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    int quantum = current_quantum;
    if (quantum < 1) quantum = 1;

    int queue[2000];
    int q_start = 0, q_end = 0;
    int in_queue[100] = {0}; 

    for(int i=0; i<count; i++) {
        if (procs[i].arrival_time == 0) { queue[q_end++] = i; in_queue[i] = 1; }
    }

    int current_proc_idx = -1;
    int quantum_counter = 0;

    while (completed < count) {
        // Gestion IO (Retour en queue)
        for (int i = 0; i < count; i++) {
            if (procs[i].in_io) {
                procs[i].io_remaining--;
                if (procs[i].io_remaining <= 0) {
                    procs[i].in_io = 0; procs[i].current_io_idx++;
                    if (!procs[i].finished) { queue[q_end++] = i; in_queue[i] = 1; }
                }
            }
        }
        // Nouveaux arrivants
        for (int i = 0; i < count; i++) {
            if (!in_queue[i] && !procs[i].finished && !procs[i].in_io && procs[i].arrival_time == current_time) {
                queue[q_end++] = i; in_queue[i] = 1;
            }
        }

        if (current_proc_idx == -1 && q_start < q_end) {
            current_proc_idx = queue[q_start++];
            in_queue[current_proc_idx] = 0; 
            quantum_counter = 0;
        }

        if (current_proc_idx != -1) {
            VisProcess *p = &procs[current_proc_idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            quantum_counter++;

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

// 3. SJF (Non-Préemptif)
void run_sjf(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    int current_proc_idx = -1;

    while (completed < count) {
        handle_io_background(procs, count);

        if (current_proc_idx == -1) {
            int idx = -1;
            int min_dur = INT_MAX;
            for (int i = 0; i < count; i++) {
                if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                    if (procs[i].duration < min_dur) {
                        min_dur = procs[i].duration;
                        idx = i;
                    }
                }
            }
            current_proc_idx = idx;
        }

        if (current_proc_idx != -1) {
            VisProcess *p = &procs[current_proc_idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            if (check_start_io(p)) current_proc_idx = -1;
            else if (p->remaining_time == 0) { p->finished = 1; completed++; current_proc_idx = -1; }
        }
        current_time++;
    }
}

// 4. Priority (Préemptif)
void run_priority(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;

    while (completed < count) {
        handle_io_background(procs, count);

        int idx = -1;
        int best_prio = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                // Config: Priority 1 is high, 5 is low
                if (procs[i].priority < best_prio) { best_prio = procs[i].priority; idx = i; }
            }
        }

        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            check_start_io(p); 
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

// 5. Multilevel Static (Simulé comme Prio Préemptive)
void run_multilevel_static(VisProcess procs[], int count) { run_priority(procs, count); }

// 6. Multilevel Aging
void run_multilevel_aging(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        // Aging
        for(int i=0; i<count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                procs[i].wait_time++;
                if(procs[i].wait_time >= 5 && procs[i].priority > 1) {
                    procs[i].priority--; procs[i].wait_time = 0;
                }
            }
        }
        // Selection
        int idx = -1;
        int best_prio = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].priority < best_prio) { best_prio = procs[i].priority; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            p->wait_time = 0; 
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

// 7. SRT
void run_srt(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    while (completed < count) {
        handle_io_background(procs, count);
        int idx = -1;
        int min_rem = INT_MAX;
        for (int i = 0; i < count; i++) {
            if (!procs[i].finished && !procs[i].in_io && procs[i].arrival_time <= current_time) {
                if (procs[i].remaining_time < min_rem) { min_rem = procs[i].remaining_time; idx = i; }
            }
        }
        if (idx != -1) {
            VisProcess *p = &procs[idx];
            add_slice(p->id, current_time, 1, p->color);
            p->remaining_time--;
            p->executed_time++;
            check_start_io(p);
            if (p->remaining_time == 0) { p->finished = 1; completed++; }
        }
        current_time++;
    }
}

void simulate_current_algo() {
    slice_count = 0;
    // Copie de travail pour ne pas modifier les données initiales
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

// --- Affichage ---

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

    // Redimensionnement dynamique pour le Scroll
    int needed_width = start_x + (max_end * scale) + 50;
    if (needed_width > width) gtk_drawing_area_set_content_width(area, needed_width);

    // Fond
    cairo_set_source_rgb(cr, 0.18, 0.20, 0.23); 
    cairo_paint(cr);

    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);

    // Grille
    for (int t = 0; t <= max_end; t++) {
        double x = start_x + (t * scale);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.3); 
        cairo_move_to(cr, x, 30); cairo_line_to(cr, x, axis_y); cairo_stroke(cr);
        
        cairo_set_source_rgb(cr, 1, 1, 1);
        char num[20]; sprintf(num, "%d", t);
        cairo_move_to(cr, x - 4, axis_y + 15);
        cairo_show_text(cr, num);
    }
    
    // Axe
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, start_x, axis_y);
    cairo_line_to(cr, start_x + (max_end * scale), axis_y);
    cairo_stroke(cr);

    // Slices
    for (int i = 0; i < slice_count; i++) {
        GanttSlice s = slices[i];
        
        double x = start_x + (s.start * scale);
        double w = s.duration * scale;
        double y = axis_y - 60;
        double h = 40;

        GdkRGBA color; gdk_rgba_parse(&color, s.color);
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1.0);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill_preserve(cr);
        
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_set_line_width(cr, 1);
        cairo_stroke(cr);

        if (w > 20) {
            cairo_set_source_rgb(cr, 0, 0, 0);
            char buf[20]; sprintf(buf, "%s", s.pid);
            cairo_move_to(cr, x + 5, y + 25);
            cairo_show_text(cr, buf);
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

static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window { background-color: #282c34; }"
        "label { color: white; }" 
        "dropdown { background-color: #f0f0f0; color: black; }"
        "dropdown label { color: black; }" 
        "dropdown arrow { color: black; }"
        "popover { background-color: white; }"
        "popover listview { color: black; }"
        "popover label { color: black; }"; 

    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    load_css();
    
    // CHARGEMENT DE LA CONFIG (Chemin relatif supposé correct)
    load_config_file("Config/config.txt");

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simulateur OS - GTK4");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 600);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(window), paned);
    gtk_paned_set_position(GTK_PANED(paned), 350);

    // Sidebar
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(sidebar, 10);
    gtk_widget_set_margin_top(sidebar, 10);
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);

    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("Algorithme :"));
    const char *algos[] = {
        "1. FCFS", "2. Round Robin", "3. SJF (Non-Preemptif)", 
        "4. Preemptive Priority", "5. Multilevel Static", 
        "6. Multilevel Aging", "7. SRT", NULL
    };
    GtkWidget *dropdown = gtk_drop_down_new_from_strings(algos);
    g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_algo_changed), NULL);
    gtk_box_append(GTK_BOX(sidebar), dropdown);

    // Quantum
    quantum_control_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_append(GTK_BOX(quantum_control_box), gtk_label_new("Quantum (RR) :"));
    GtkWidget *spin = gtk_spin_button_new_with_range(1, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 3);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_append(GTK_BOX(quantum_control_box), spin);
    gtk_box_append(GTK_BOX(sidebar), quantum_control_box);
    gtk_widget_set_visible(quantum_control_box, FALSE); 

    // Légende Dynamique (Basée sur le fichier chargé)
    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("\n--- Légende Config ---"));
    for(int i=0; i<total_processes; i++) {
        char buf[100];
        sprintf(buf, "%s : Arr %d | Dur %d | Prio %d", 
            initial_processes[i].id, initial_processes[i].arrival_time, 
            initial_processes[i].duration, initial_processes[i].priority);
        GtkWidget *l = gtk_label_new(buf);
        gtk_box_append(GTK_BOX(sidebar), l);
    }

    // Scroll + Drawing
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, -1, 400); 
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_function, NULL, NULL);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), drawing_area);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    
    gtk_paned_set_end_child(GTK_PANED(paned), scrolled_window);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.projetse.final", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
