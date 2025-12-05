#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// --- Structures de Données ---

// Représente un processus à simuler
typedef struct {
    int pid;
    int arrival_time;
    int duration;
    int remaining_time;
    int priority;       // 1 = Haute, 5 = Basse
    int wait_time;      // Pour l'Aging
    const char *color;
} VisProcess;

// Représente une "tranche" visuelle sur le Gantt (Un bloc de couleur)
typedef struct {
    int pid;
    double start;
    double duration;
    const char *color;
} GanttSlice;

// --- Données Globales ---

#define MAX_SLICES 1000
GanttSlice slices[MAX_SLICES];
int slice_count = 0;

VisProcess initial_processes[] = {
    {1, 0,  10, 10, 3, 0, "#E06C75"}, // Rouge
    {2, 2,  6,  6,  1, 0, "#98C379"}, // Vert (Prio Haute)
    {3, 4,  8,  8,  4, 0, "#61AFEF"}, // Bleu
    {4, 6,  4,  4,  2, 0, "#E5C07B"}, // Jaune
    {5, 8,  5,  5,  3, 0, "#C678DD"}  // Violet
};
int total_processes = 5;

static int current_algo_index = 0;
static GtkWidget *drawing_area; 

// --- Moteur de Simulation ---

// Ajoute une tranche au graphique
void add_slice(int pid, int start, int duration, const char* color) {
    if (slice_count >= MAX_SLICES) return;
    
    // Si le dernier slice est le même processus, on le prolonge (fusion visuelle)
    if (slice_count > 0 && slices[slice_count-1].pid == pid && 
        (slices[slice_count-1].start + slices[slice_count-1].duration) == start) {
        slices[slice_count-1].duration += duration;
    } else {
        slices[slice_count].pid = pid;
        slices[slice_count].start = start;
        slices[slice_count].duration = duration;
        slices[slice_count].color = color;
        slice_count++;
    }
}

// 1. FCFS (Premier Arrivé Premier Servi)
void run_fcfs(VisProcess procs[], int count) {
    int current_time = 0;
    // Tri simple par arrivée (Bubble sort pour l'exemple)
    for(int i=0; i<count-1; i++) {
        for(int j=0; j<count-i-1; j++) {
            if(procs[j].arrival_time > procs[j+1].arrival_time) {
                VisProcess temp = procs[j]; procs[j] = procs[j+1]; procs[j+1] = temp;
            }
        }
    }
    for (int i = 0; i < count; i++) {
        if (current_time < procs[i].arrival_time) current_time = procs[i].arrival_time;
        add_slice(procs[i].pid, current_time, procs[i].duration, procs[i].color);
        current_time += procs[i].duration;
    }
}

// 2. Round Robin (Quantum = 3)
void run_rr(VisProcess procs[], int count) {
    int quantum = 3;
    int current_time = 0;
    int completed = 0;
    
    while (completed < count) {
        int progress = 0;
        for (int i = 0; i < count; i++) {
            if (procs[i].remaining_time > 0 && procs[i].arrival_time <= current_time) {
                int run_time = (procs[i].remaining_time > quantum) ? quantum : procs[i].remaining_time;
                
                add_slice(procs[i].pid, current_time, run_time, procs[i].color);
                
                procs[i].remaining_time -= run_time;
                current_time += run_time;
                progress = 1;

                if (procs[i].remaining_time == 0) completed++;
            }
        }
        if (!progress) current_time++; // CPU Idle
    }
}

// 3. SJF (Non-Préemptif)
void run_sjf(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    
    while (completed < count) {
        int idx = -1;
        int min_dur = INT_MAX;

        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].duration < min_dur) {
                    min_dur = procs[i].duration;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            add_slice(procs[idx].pid, current_time, procs[idx].duration, procs[idx].color);
            current_time += procs[idx].duration;
            procs[idx].remaining_time = 0;
            completed++;
        } else {
            current_time++;
        }
    }
}

// 4. Preemptive Priority
void run_priority(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;

    while (completed < count) {
        int idx = -1;
        int best_prio = INT_MAX; // 1 est meilleur que 10

        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].priority < best_prio) {
                    best_prio = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            add_slice(procs[idx].pid, current_time, 1, procs[idx].color);
            procs[idx].remaining_time--;
            current_time++;
            if (procs[idx].remaining_time == 0) completed++;
        } else {
            current_time++;
        }
    }
}

// 5. Multilevel Static (Prio + RR Quantum 2)
void run_multilevel_static(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    int quantum = 2; // Spécifique à cet algo

    while (completed < count) {
        int idx = -1;
        int best_prio = INT_MAX;

        // Trouver la file prioritaire active
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].priority < best_prio) {
                    best_prio = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            // Exécuter Quantum 2 ou reste
            int run = (procs[idx].remaining_time > quantum) ? quantum : procs[idx].remaining_time;
            add_slice(procs[idx].pid, current_time, run, procs[idx].color);
            procs[idx].remaining_time -= run;
            current_time += run;
            if (procs[idx].remaining_time == 0) completed++;
        } else {
            current_time++;
        }
    }
}

// 6. Multilevel Aging (Attente -> Prio augmente)
void run_multilevel_aging(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;
    int quantum = 2;

    while (completed < count) {
        int idx = -1;
        int best_prio = INT_MAX;

        // Sélection
        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].priority < best_prio) {
                    best_prio = procs[i].priority;
                    idx = i;
                }
            }
        }

        // Aging : Augmenter priorité des autres (wait_time)
        for(int i=0; i<count; i++) {
            if (i != idx && procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                procs[i].wait_time++;
                if(procs[i].wait_time >= 5 && procs[i].priority > 1) { // Tous les 5 ticks
                    procs[i].priority--; // Améliorer priorité
                    procs[i].wait_time = 0;
                }
            }
        }

        if (idx != -1) {
            int run = (procs[idx].remaining_time > quantum) ? quantum : procs[idx].remaining_time;
            add_slice(procs[idx].pid, current_time, run, procs[idx].color);
            procs[idx].remaining_time -= run;
            current_time += run;
            procs[idx].wait_time = 0; // Reset wait si exécuté
            if (procs[idx].remaining_time == 0) completed++;
        } else {
            current_time++;
        }
    }
}

// 7. SRT (Shortest Remaining Time - Préemptif)
void run_srt(VisProcess procs[], int count) {
    int current_time = 0;
    int completed = 0;

    while (completed < count) {
        int idx = -1;
        int min_rem = INT_MAX;

        for (int i = 0; i < count; i++) {
            if (procs[i].arrival_time <= current_time && procs[i].remaining_time > 0) {
                if (procs[i].remaining_time < min_rem) {
                    min_rem = procs[i].remaining_time;
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            add_slice(procs[idx].pid, current_time, 1, procs[idx].color);
            procs[idx].remaining_time--;
            current_time++;
            if (procs[idx].remaining_time == 0) completed++;
        } else {
            current_time++;
        }
    }
}

// Fonction Dispatcher
void simulate_current_algo() {
    slice_count = 0;
    
    // Copie fraîche des processus
    VisProcess working_procs[5];
    for(int i=0; i<5; i++) working_procs[i] = initial_processes[i];

    switch(current_algo_index) {
        case 0: run_fcfs(working_procs, 5); break;
        case 1: run_rr(working_procs, 5); break;
        case 2: run_sjf(working_procs, 5); break;
        case 3: run_priority(working_procs, 5); break;
        case 4: run_multilevel_static(working_procs, 5); break;
        case 5: run_multilevel_aging(working_procs, 5); break;
        case 6: run_srt(working_procs, 5); break;
    }
}

// --- Affichage (Cairo) ---

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    simulate_current_algo(); // Recalculer le Gantt

    cairo_set_source_rgb(cr, 0.18, 0.20, 0.23); 
    cairo_paint(cr);

    // Axe Temporel
    double start_x = 40;
    double axis_y = height - 80;
    double scale = 20.0; // Zoom plus grand pour voir les détails

    // Trouver fin max
    double max_end = 0;
    for(int i=0; i<slice_count; i++) 
        if((slices[i].start + slices[i].duration) > max_end) 
            max_end = slices[i].start + slices[i].duration;
    max_end += 5;

    // Dessin Grille
    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    for (int t = 0; t <= max_end; t++) {
        double x = start_x + (t * scale);
        
        if (t % 5 == 0) { // Grand tick
            cairo_set_source_rgba(cr, 1, 1, 1, 0.2); 
            cairo_move_to(cr, x, 50); cairo_line_to(cr, x, axis_y); cairo_stroke(cr);
            
            cairo_set_source_rgb(cr, 1, 1, 1);
            char num[20]; sprintf(num, "%d", t);
            cairo_move_to(cr, x - 5, axis_y + 15);
            cairo_show_text(cr, num);
        } else { // Petit tick
            cairo_set_source_rgba(cr, 1, 1, 1, 0.1); 
            cairo_move_to(cr, x, axis_y - 5); cairo_line_to(cr, x, axis_y); cairo_stroke(cr);
        }
    }
    
    // Axe
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, start_x, axis_y);
    cairo_line_to(cr, start_x + (max_end * scale), axis_y);
    cairo_stroke(cr);

    // Dessin des Slices
    for (int i = 0; i < slice_count; i++) {
        GanttSlice s = slices[i];
        
        double x = start_x + (s.start * scale);
        double w = s.duration * scale;
        double y = axis_y - 50;
        double h = 40;

        GdkRGBA color; gdk_rgba_parse(&color, s.color);
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1.0);
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill_preserve(cr);
        
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_set_line_width(cr, 1);
        cairo_stroke(cr);

        // Texte PID si assez de place
        if (w > 20) {
            cairo_set_source_rgb(cr, 0, 0, 0);
            char buf[10]; sprintf(buf, "P%d", s.pid);
            cairo_move_to(cr, x + 5, y + 25);
            cairo_show_text(cr, buf);
        }
    }
}

static void on_algo_changed(GObject *object, GParamSpec *pspec, gpointer data) {
    GtkDropDown *dropdown = GTK_DROP_DOWN(object);
    current_algo_index = gtk_drop_down_get_selected(dropdown);
    if (drawing_area) gtk_widget_queue_draw(drawing_area);
}

static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        // 1. Fond sombre pour l'application
        "window { background-color: #282c34; }"
        
        // 2. Texte blanc PAR DÉFAUT pour toute l'app
        "label { color: white; }"

        // 3. --- CONFIGURATION DU DROPDOWN ---
        
        // Le bouton lui-même (fond clair)
        "dropdown { background-color: #f0f0f0; color: black; }"
        
        // CORRECTION ICI : Force le texte À L'INTÉRIEUR du bouton en noir
        "dropdown label { color: black; }"
        
        // La petite flèche
        "dropdown arrow { color: black; }"
        
        // Le menu déroulant (la liste ouverte)
        "popover { background-color: white; }"
        "popover listview { color: black; }"
        "popover label { color: black; }"; // Les items de la liste

    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    load_css();
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simulateur Complet d'Ordonnancement");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(window), paned);
    gtk_paned_set_position(GTK_PANED(paned), 300);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(sidebar, 10);
    gtk_widget_set_margin_top(sidebar, 10);
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar);

    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("Algorithme :"));
    const char *algos[] = {
        "1. FCFS", 
        "2. Round Robin (Q=3)", 
        "3. SJF (Non-Preemptif)", 
        "4. Preemptive Priority", 
        "5. Multilevel Static", 
        "6. Multilevel Aging", 
        "7. SRT (Shortest Remaining Time)", 
        NULL
    };
    GtkWidget *dropdown = gtk_drop_down_new_from_strings(algos);
    g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_algo_changed), NULL);
    gtk_box_append(GTK_BOX(sidebar), dropdown);

    // Légende des Processus
    gtk_box_append(GTK_BOX(sidebar), gtk_label_new("\n--- Légende ---"));
    for(int i=0; i<5; i++) {
        char buf[100];
        sprintf(buf, "P%d : Arrivée %d | Durée %d | Prio %d", 
            initial_processes[i].pid, initial_processes[i].arrival_time, 
            initial_processes[i].duration, initial_processes[i].priority);
        GtkWidget *l = gtk_label_new(buf);
        // Colorer le texte (Astuce CSS simple)
        // Pour faire simple ici, on garde le texte blanc
        gtk_box_append(GTK_BOX(sidebar), l);
    }

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_function, NULL, NULL);
    gtk_paned_set_end_child(GTK_PANED(paned), drawing_area);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.projetse.full", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
