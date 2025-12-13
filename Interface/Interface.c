/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../Utils/utils.h"
#include "./interface_utils.h"
#include "./gantt_chart.h"  
#include "../Config/config.h"
#include "../Config/types.h"
#include "../Utils/Algorithms.h"
#include "../Utils/log_file.h"

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256
void update_process_list_ui(AppContext *app) {
    GtkWidget *list_box = app->process_list_box;
    Config *CFG = app->CFG;

    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(list_box))) != NULL) {
        gtk_list_box_remove(GTK_LIST_BOX(list_box), child);
    }

    if (CFG->process_count <= 0) {
        GtkWidget *empty_label = gtk_label_new("No processes loaded or file not found.");
        gtk_widget_add_css_class(empty_label, "process-row");
        gtk_list_box_append(GTK_LIST_BOX(list_box), empty_label);
        return;
    }

    char status_text[128];
    snprintf(status_text, sizeof(status_text), "Loaded %d processes", CFG->process_count);
    GtkWidget *status_label = gtk_label_new(status_text);
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(status_label, "list-header");
    gtk_list_box_append(GTK_LIST_BOX(list_box), status_label);

    for (int i = 0; i < CFG->process_count; i++) {
        char buffer[2048] = {0};
        PROCESS p = CFG->processes[i];

        int offset = 0;
        int remaining = sizeof(buffer) - 1;

        offset += snprintf(buffer + offset, remaining, "<b>ID: %s</b>", p.ID);
        remaining = sizeof(buffer) - 1 - offset;

        if (remaining > 0) {
            offset += snprintf(buffer + offset, remaining, "\nArrival: %d | Exec: %d | Prio: %d", 
                             p.arrival_time, p.execution_time, p.priority);
        }

        GtkWidget *row_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(row_label), buffer);
        gtk_label_set_wrap(GTK_LABEL(row_label), TRUE);
        gtk_widget_set_halign(row_label, GTK_ALIGN_START);
        gtk_widget_add_css_class(row_label, "process-row");

        gtk_list_box_append(GTK_LIST_BOX(list_box), row_label);
    }
}

void handle_config_submission(AppContext *app, const char *filename) {
    if (!filename || strlen(filename) == 0) {
        g_print("Error: Empty filename\n");
        return;
    }

    char full_path[512];
    if (strstr(filename, "/") == NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", CONFIG_DIR, filename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s", filename);
    }

    g_print("Attempting to load: %s\n", full_path);
    strncpy(app->config_filename, filename, sizeof(app->config_filename) - 1);
    app->config_filename[sizeof(app->config_filename) - 1] = '\0';

    int res = load_config(full_path, app->CFG);

    if (res == 1) {
        g_print("Success.\n");
        gtk_widget_set_sensitive(app->edit_config_btn, TRUE);
        update_process_list_ui(app);
    } else {
        g_print("Failed to load config.\n");
        gtk_widget_set_sensitive(app->edit_config_btn, FALSE);
        app->CFG->process_count = 0;
        update_process_list_ui(app);
    }
}

static void on_submit_clicked(GtkWidget *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(app->config_entry));
    handle_config_submission(app, text);
}

static void on_browse_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    GtkWidget *label = gtk_list_box_row_get_child(row);
    const char *filename = gtk_label_get_text(GTK_LABEL(label));

    gtk_editable_set_text(GTK_EDITABLE(app->config_entry), filename);
    handle_config_submission(app, filename);

    GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_native(GTK_WIDGET(box)));
    gtk_window_close(GTK_WINDOW(toplevel));
}

static void on_browse_clicked(GtkWidget *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    scan_config_directory(app);

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Select Config File");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 400);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content_box, 10);
    gtk_widget_set_margin_bottom(content_box, 10);
    gtk_widget_set_margin_start(content_box, 10);
    gtk_widget_set_margin_end(content_box, 10);
    gtk_window_set_child(GTK_WINDOW(dialog), content_box);

    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
    g_signal_connect(list_box, "row-activated", G_CALLBACK(on_browse_row_activated), app);

    for (int i = 0; i < app->files_count; i++) {
        GtkWidget *row_lbl = gtk_label_new(app->available_files[i]);
        gtk_widget_set_halign(row_lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_start(row_lbl, 10);
        gtk_widget_set_margin_top(row_lbl, 10);
        gtk_widget_set_margin_bottom(row_lbl, 10);
        gtk_list_box_append(GTK_LIST_BOX(list_box), row_lbl);
    }

    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), list_box);

    gtk_box_append(GTK_BOX(content_box), scroller);

    GtkWidget *close_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(content_box), close_btn);

    gtk_window_present(GTK_WINDOW(dialog));
}


static gboolean algorithm_requires_quantum(const char *algorithm) {
        return (strcmp(algorithm, "Round_Robin") == 0 ||
            strcmp(algorithm, "Multilevel_Aging") == 0 ||
            strcmp(algorithm, "MultilevelAging") == 0 ||
            strcmp(algorithm, "Multilevel_Static") == 0);
}

static gboolean algorithm_requires_aging(const char *algorithm) {
        return (strcmp(algorithm, "Multilevel_Aging") == 0 ||
            strcmp(algorithm, "MultilevelAging") == 0);
}



static void on_algorithm_selected(GObject *dropdown, GParamSpec *pspec, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(dropdown)));
    guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));

    if (index != GTK_INVALID_LIST_POSITION) {
        const char *algorithm = gtk_string_list_get_string(list, index);
        g_print("Selected algorithm: %s\n", algorithm);

        // Show or hide quantum input based on algorithm
        if (algorithm_requires_quantum(algorithm)) {
            gtk_widget_set_visible(app->quantum_box, TRUE);
        } else {
            gtk_widget_set_visible(app->quantum_box, FALSE);
        }
        
        // Show or hide aging parameters based on algorithm
        if (algorithm_requires_aging(algorithm)) {
            gtk_widget_set_visible(app->aging_interval_box, TRUE);
            gtk_widget_set_visible(app->max_priority_box, TRUE);
        } else {
            gtk_widget_set_visible(app->aging_interval_box, FALSE);
            gtk_widget_set_visible(app->max_priority_box, FALSE);
        }
    }
}

static void on_algorithm_file_added(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GFile *file = gtk_file_dialog_open_finish(dialog, result, NULL);
    AppContext *app = (AppContext *)user_data;

    if (file) {
        char *filename = g_file_get_basename(file);
        char dest_path[512];
        snprintf(dest_path, sizeof(dest_path), "Algorithms/%s", filename);

        GFile *dest_file = g_file_new_for_path(dest_path);
        GError *error = NULL;

        if (g_file_copy(file, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            char *ext = strrchr(filename, '.');
            if (ext) {
                int len = ext - filename;
                char algo_name[256];
                strncpy(algo_name, filename, len);
                algo_name[len] = '\0';

                GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(app->algo_dropdown)));
                gtk_string_list_append(list, algo_name);
            }
        }
        g_object_unref(dest_file);
        g_free(filename);
        g_object_unref(file);
    }
}

void on_logfile_clicked(GtkWidget *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Logfile Viewer");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(content_box, 10);
    gtk_widget_set_margin_bottom(content_box, 10);
    gtk_widget_set_margin_start(content_box, 10);
    gtk_widget_set_margin_end(content_box, 10);
    gtk_window_set_child(GTK_WINDOW(dialog), content_box);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    FILE *log_file = fopen(app->log_filename, "r");
    if (log_file) {
        fseek(log_file, 0, SEEK_END);
        long fsize = ftell(log_file);
        fseek(log_file, 0, SEEK_SET);

        char *log_content = malloc(fsize + 1);
        if (log_content) {
            fread(log_content, 1, fsize, log_file);
            log_content[fsize] = '\0';
            fclose(log_file);

            gtk_text_buffer_set_text(buffer, log_content, -1);
            free(log_content);
        } else {
            fclose(log_file);
            gtk_text_buffer_set_text(buffer, "Failed to allocate memory for logfile.", -1);
        }
    } else {
        gtk_text_buffer_set_text(buffer, "Logfile not found.", -1);
    }

    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text_view);
    
    gtk_box_append(GTK_BOX(content_box), scroller);
    
    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(content_box), close_btn);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_add_algorithm_clicked(GtkButton *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.c");
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    gtk_file_dialog_open(dialog, GTK_WINDOW(app->window), NULL, on_algorithm_file_added, app);
}


static void on_start_clicked(GtkButton *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    if (app->CFG->process_count <= 0) {
        g_print("Error: No processes loaded\n");
        return;
    }

    // Get selected algorithm
    GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(app->algo_dropdown)));
    guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(app->algo_dropdown));

    if (index == GTK_INVALID_LIST_POSITION) {
        g_print("Error: No algorithm selected\n");
        return;
    }

    const char *algorithm = gtk_string_list_get_string(list, index);

    g_print("Starting scheduler with algorithm: %s\n", algorithm);
    g_print("Processes loaded: %d\n", app->CFG->process_count);

    // Prepare log filename
    char config_copy[256];
    strncpy(config_copy, app->config_filename, sizeof(config_copy) - 1);
    remove_extension(config_copy);
    snprintf(app->log_filename, sizeof(app->log_filename), "output/%s_%s.log", algorithm, config_copy);

    init_log(algorithm, app->config_filename);

    // Get quantum value if needed
    int quantum = 2; // Default value
    if (algorithm_requires_quantum(algorithm)) {
        const char *quantum_text = gtk_editable_get_text(GTK_EDITABLE(app->quantum_entry));
        if (quantum_text && strlen(quantum_text) > 0) {
            quantum = atoi(quantum_text);
            if (quantum <= 0) {
                g_print("Warning: Invalid quantum value, using default (2)\n");
                quantum = 2;
            }
        }
        g_print("Quantum: %d\n", quantum);
    }
    int aging_interval = 3; // Default
    int max_priority = 5;   // Default
    
    if (algorithm_requires_aging(algorithm)) {
        const char *aging_text = gtk_editable_get_text(GTK_EDITABLE(app->aging_interval_entry));
        if (aging_text && strlen(aging_text) > 0) {
            aging_interval = atoi(aging_text);
            if (aging_interval <= 0) {
                g_print("Warning: Invalid aging interval, using default (3)\n");
                aging_interval = 3;
            }
        }
        
        const char *priority_text = gtk_editable_get_text(GTK_EDITABLE(app->max_priority_entry));
        if (priority_text && strlen(priority_text) > 0) {
            max_priority = atoi(priority_text);
            if (max_priority <= 0) {
                g_print("Warning: Invalid max priority, using default (5)\n");
                max_priority = 5;
            }
        }
        
        g_print("Aging Interval: %d, Max Priority: %d\n", aging_interval, max_priority);
    }

    // Store quantum in app context
    app->quantum = quantum;

    // Clear previous Gantt data before running algorithm
    clear_gantt_slices();
    clear_io_slices();  // NEW: Clear I/O slices

    // Call the appropriate scheduling algorithm based on selection
    if (strcmp(algorithm, "First_In_First_Out") == 0) {
        FCFS_Algo(app->CFG);
    } 
    else if (strcmp(algorithm, "Round_Robin") == 0) {
        RoundRobin_Algo(app->CFG, quantum);
    }
    else if (strcmp(algorithm, "Multilevel_Aging") == 0 || strcmp(algorithm, "MultilevelAging") == 0) {
        MultilevelAgingScheduler(app->CFG, quantum, aging_interval, max_priority);
    }
    else if (strcmp(algorithm, "Preemptive_Priority") == 0) {
        run_priority_preemptive(app->CFG);
    }
    else if (strcmp(algorithm, "Shortest_Job_First") == 0) {
        SJF_Algo(app->CFG);
    }
    else if (strcmp(algorithm, "Multilevel_Static") == 0) {
        MultilevelStaticScheduler(app->CFG, quantum);
    }
    else if (strcmp(algorithm, "Shortest_Remaining_Time") == 0) {
        SRT_Algo(app->CFG);
    }
    else {
        g_print("Warning: Unknown algorithm '%s'\n", algorithm);
    }

    // Refresh the Gantt chart after algorithm completes
    gtk_widget_queue_draw(app->gantt_widget);

    g_print("Scheduling complete. CPU slices: %d, I/O slices: %d\n", slice_count, io_slice_count);

    gtk_widget_set_sensitive(app->show_logfile_btn, TRUE);
}
static void on_add_process_clicked(GtkButton *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    if (app->CFG->process_count >= 20) {
        // Maximum processes reached
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_WARNING,
                                                GTK_BUTTONS_OK,
                                                "Maximum 20 processes allowed.");
        gtk_window_present(GTK_WINDOW(msg));
        return;
    }

    // Add a new empty process in CFG
    PROCESS *new_process = &app->CFG->processes[app->CFG->process_count];
    strcpy(new_process->ID, "P?");
    new_process->arrival_time = 0;
    new_process->execution_time = 0;
    new_process->priority = 0;
    new_process->io_count = 0;

    int index = app->CFG->process_count;
    app->CFG->process_count++;

    // Add a new row to the GTK list
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(new_process->ID);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    if (app->editor_process_list_box)
        gtk_list_box_append(GTK_LIST_BOX(app->editor_process_list_box), row);
    else
        gtk_list_box_append(GTK_LIST_BOX(app->process_list_box), row);

    gtk_widget_show(row);
}


void on_edit_config_clicked(GtkButton *button, gpointer user_data)
{
    AppContext *app = (AppContext *)user_data;

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Edit Configuration");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                GTK_WINDOW(app->window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 500);

    /* Main vertical container */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_top(main_box, 15);
    gtk_widget_set_margin_bottom(main_box, 15);
    gtk_widget_set_margin_start(main_box, 15);
    gtk_widget_set_margin_end(main_box, 15);
    gtk_window_set_child(GTK_WINDOW(dialog), main_box);

    /* Title */
    GtkWidget *title = gtk_label_new("Edit Processes (in-memory)");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(main_box), title);

    /* Horizontal split: list | editor */
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_append(GTK_BOX(main_box), content);

    /* LEFT: process list (scrollable) */
    GtkWidget *list_scroller = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(list_scroller, TRUE);
    gtk_widget_set_vexpand(list_scroller, TRUE);

    GtkWidget *process_list = create_process_list_editor(app);
    app->editor_process_list_box = process_list;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroller),
                                  process_list);
    gtk_box_append(GTK_BOX(content), list_scroller);

    /* RIGHT: editor form */
    GtkWidget *editor = create_editor_form(app);
    gtk_widget_set_hexpand(editor, TRUE);
    gtk_box_append(GTK_BOX(content), editor);


    /* Footer buttons */
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(footer, GTK_ALIGN_END);

    GtkWidget *add_btn = gtk_button_new_with_label("Add Process");
    gtk_box_append(GTK_BOX(footer), add_btn);

    /* Connect the signal */
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_process_clicked), app);

    GtkWidget *apply_btn = gtk_button_new_with_label("Apply");
    GtkWidget *delete_btn = gtk_button_new_with_label("Delete");
    GtkWidget *close_btn = gtk_button_new_with_label("Close");

    gtk_box_append(GTK_BOX(footer), apply_btn);
    gtk_box_append(GTK_BOX(footer), delete_btn);
    gtk_box_append(GTK_BOX(footer), close_btn);

    gtk_box_append(GTK_BOX(main_box), footer);

    g_signal_connect(apply_btn, "clicked",
                    G_CALLBACK(on_apply_process_changes), app);
    g_signal_connect(delete_btn, "clicked",
                    G_CALLBACK(on_delete_process_clicked), app);

    g_signal_connect_swapped(close_btn, "clicked",
                            G_CALLBACK(gtk_window_close), dialog);
    gtk_window_present(GTK_WINDOW(dialog));

}

static void refresh_process_list(AppContext *app)
{
    if (!app || !app->CFG || !app->editor_process_list_box) return;

    GtkListBox *box = GTK_LIST_BOX(app->editor_process_list_box);

    /* Remove all rows */
    GtkWidget *row;
    while ((row = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
        gtk_list_box_remove(box, row);
    }

    /* Rebuild rows */
    for (int i = 0; i < app->CFG->process_count; i++) {
        PROCESS p = app->CFG->processes[i];

        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "%s | Arr:%d Exec:%d Pr:%d IO:%d",
                 p.ID,
                 p.arrival_time,
                 p.execution_time,
                 p.priority,
                 p.io_count);

        GtkWidget *label = gtk_label_new(buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_list_box_append(box, label);
    }
}


static void on_apply_process_changes(GtkButton *button,
                                     gpointer user_data)
{
    AppContext *app = (AppContext *)user_data;

    if (!app || !app->CFG) return;
    if (app->selected_process < 0 ||
        app->selected_process >= app->CFG->process_count)
        return;

    PROCESS *p = &app->CFG->processes[app->selected_process];

    /* Read fields */
    const char *id = gtk_editable_get_text(GTK_EDITABLE(app->id_entry));
    const char *arrival = gtk_editable_get_text(GTK_EDITABLE(app->arrival_entry));
    const char *exec = gtk_editable_get_text(GTK_EDITABLE(app->exec_entry));
    const char *priority = gtk_editable_get_text(GTK_EDITABLE(app->priority_entry));
    const char *io = gtk_editable_get_text(GTK_EDITABLE(app->io_entry));

    /* Basic validation */
    if (!id || !*id) return;

    int arrival_i = atoi(arrival);
    int exec_i = atoi(exec);
    int priority_i = atoi(priority);
    int io_i = atoi(io);

    if (arrival_i < 0 || exec_i <= 0 || priority_i < 0 || io_i < 0)
        return;

    /* Apply to CFG */
    strncpy(p->ID, id, sizeof(p->ID) - 1);
    p->ID[sizeof(p->ID) - 1] = '\0';

    p->arrival_time = arrival_i;
    p->execution_time = exec_i;
    p->priority = priority_i;
    p->io_count = io_i;

    refresh_process_list(app);

    if (app->editor_process_list_box) {
        gtk_list_box_select_row(
            GTK_LIST_BOX(app->editor_process_list_box),
            gtk_list_box_get_row_at_index(
                GTK_LIST_BOX(app->editor_process_list_box),
                app->selected_process
            )
        );
    }

    /* Persist changes to the config file if one is loaded, then reload */
    if (app->config_filename && app->config_filename[0] != '\0') {
        char full_path[512];
        if (strstr(app->config_filename, "/") == NULL) {
            snprintf(full_path, sizeof(full_path), "%s/%s", CONFIG_DIR, app->config_filename);
        } else {
            snprintf(full_path, sizeof(full_path), "%s", app->config_filename);
        }

        if (save_config(full_path, app->CFG)) {
            g_print("Config saved to %s\n", full_path);
            /* Reload the config to ensure main window reflects file contents */
            handle_config_submission(app, app->config_filename);
        } else {
            g_print("Failed to save config to %s\n", full_path);
        }
    }
}


static void load_process_into_editor(AppContext *app, int index) {
    if (!app || !app->CFG) return;
    if (index < 0 || index >= app->CFG->process_count) return;

    PROCESS p = app->CFG->processes[index];

    gtk_editable_set_text(GTK_EDITABLE(app->id_entry), p.ID);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", p.arrival_time);
    gtk_editable_set_text(GTK_EDITABLE(app->arrival_entry), buf);

    snprintf(buf, sizeof(buf), "%d", p.execution_time);
    gtk_editable_set_text(GTK_EDITABLE(app->exec_entry), buf);

    snprintf(buf, sizeof(buf), "%d", p.priority);
    gtk_editable_set_text(GTK_EDITABLE(app->priority_entry), buf);

    snprintf(buf, sizeof(buf), "%d", p.io_count);
    gtk_editable_set_text(GTK_EDITABLE(app->io_entry), buf);
}

static void on_process_row_selected(GtkListBox *box,
                                    GtkListBoxRow *row,
                                    gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    if (!row) return;

    int index = gtk_list_box_row_get_index(row);
    app->selected_process = index;

    load_process_into_editor(app, index);
}

static GtkWidget* create_process_list_editor(AppContext *app) {
    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box),
                                   GTK_SELECTION_SINGLE);

    g_signal_connect(list_box, "row-selected",
                     G_CALLBACK(on_process_row_selected), app);

    if (!app->CFG || app->CFG->process_count <= 0) {
        GtkWidget *empty = gtk_label_new("No processes loaded.");
        gtk_list_box_append(GTK_LIST_BOX(list_box), empty);
        return list_box;
    }

    for (int i = 0; i < app->CFG->process_count; i++) {
        PROCESS p = app->CFG->processes[i];

        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "%s | Arr:%d Exec:%d Pr:%d IO:%d",
                 p.ID,
                 p.arrival_time,
                 p.execution_time,
                 p.priority,
                 p.io_count);

        GtkWidget *label = gtk_label_new(buffer);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_list_box_append(GTK_LIST_BOX(list_box), label);
    }

    return list_box;
}

static GtkWidget* create_editor_form(AppContext *app) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    #define ADD_ROW(row, label_text, entry_ptr) \
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new(label_text), 0, row, 1, 1); \
        entry_ptr = gtk_entry_new(); \
        gtk_grid_attach(GTK_GRID(grid), entry_ptr, 1, row, 1, 1);

    ADD_ROW(0, "ID", app->id_entry);
    ADD_ROW(1, "Arrival Time", app->arrival_entry);
    ADD_ROW(2, "Execution Time", app->exec_entry);
    ADD_ROW(3, "Priority", app->priority_entry);
    ADD_ROW(4, "IO Count", app->io_entry);

    return grid;
}



void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    int counts = 0;
    char **algorithms = get_algorithms(&counts); 

    GtkStringList *algo_list = gtk_string_list_new(NULL);
    for (int i = 0; i < counts; i++) {
        gtk_string_list_append(algo_list, algorithms[i]);
    }

    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "OS Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 850);
    app->selected_process = -1;
    GtkWidget *main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(header_box, 30);
    gtk_widget_set_margin_bottom(header_box, 30);
    gtk_widget_set_halign(header_box, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(header_box, "header-section");
    GtkWidget *title = gtk_label_new("⚙ OS Scheduler");

    gtk_widget_add_css_class(title, "title-label");
    gtk_box_append(GTK_BOX(header_box), title);
    gtk_box_append(GTK_BOX(main_container), header_box);

    // Horizontal container for two sections
    GtkWidget *horizontal_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_margin_start(horizontal_container, 50);
    gtk_widget_set_margin_end(horizontal_container, 50);
    gtk_widget_set_margin_bottom(horizontal_container, 30);
    gtk_widget_set_hexpand(horizontal_container, TRUE);

    // Left card (Configuration) - 1/4 width
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_hexpand(card, FALSE);
    gtk_widget_set_size_request(card, 250, -1);

    gtk_widget_add_css_class(card, "card");

    // Algorithm Section
    GtkWidget *algo_label = gtk_label_new("Select Scheduling Algorithm");
    gtk_widget_set_halign(algo_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(algo_label, "section-label");
    gtk_box_append(GTK_BOX(card), algo_label);

    app->algo_dropdown = gtk_drop_down_new(G_LIST_MODEL(algo_list), NULL);
    gtk_widget_add_css_class(app->algo_dropdown, "dropdown");
    gtk_box_append(GTK_BOX(card), app->algo_dropdown);

    // Quantum input section

    // Create individual vertical boxes for each parameter (keep as before)
app->quantum_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
GtkWidget *quantum_label = gtk_label_new("Time Quantum");
gtk_widget_set_halign(quantum_label, GTK_ALIGN_START);
gtk_widget_add_css_class(quantum_label, "quantum-label");
gtk_box_append(GTK_BOX(app->quantum_box), quantum_label);
app->quantum_entry = gtk_entry_new();
gtk_entry_set_placeholder_text(GTK_ENTRY(app->quantum_entry), "Enter quantum (default: 2)");
gtk_editable_set_text(GTK_EDITABLE(app->quantum_entry), "2");
gtk_widget_add_css_class(app->quantum_entry, "quantum-input");
gtk_box_append(GTK_BOX(app->quantum_box), app->quantum_entry);
gtk_widget_set_visible(app->quantum_box, FALSE);

app->aging_interval_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
GtkWidget *aging_label = gtk_label_new("Aging Interval");
gtk_widget_set_halign(aging_label, GTK_ALIGN_START);
gtk_widget_add_css_class(aging_label, "quantum-label");
gtk_box_append(GTK_BOX(app->aging_interval_box), aging_label);
app->aging_interval_entry = gtk_entry_new();
gtk_entry_set_placeholder_text(GTK_ENTRY(app->aging_interval_entry), "Enter aging interval (default: 3)");
gtk_editable_set_text(GTK_EDITABLE(app->aging_interval_entry), "3");
gtk_widget_add_css_class(app->aging_interval_entry, "quantum-input");
gtk_box_append(GTK_BOX(app->aging_interval_box), app->aging_interval_entry);
gtk_widget_set_visible(app->aging_interval_box, FALSE);

app->max_priority_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
GtkWidget *max_priority_label = gtk_label_new("Max Priority");
gtk_widget_set_halign(max_priority_label, GTK_ALIGN_START);
gtk_widget_add_css_class(max_priority_label, "quantum-label");
gtk_box_append(GTK_BOX(app->max_priority_box), max_priority_label);
app->max_priority_entry = gtk_entry_new();
gtk_entry_set_placeholder_text(GTK_ENTRY(app->max_priority_entry), "Enter max priority (default: 5)");
gtk_editable_set_text(GTK_EDITABLE(app->max_priority_entry), "5");
gtk_widget_add_css_class(app->max_priority_entry, "quantum-input");
gtk_box_append(GTK_BOX(app->max_priority_box), app->max_priority_entry);
gtk_widget_set_visible(app->max_priority_box, FALSE);

// Create a horizontal row to hold all three parameter boxes
GtkWidget *params_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); // 10px spacing
gtk_box_append(GTK_BOX(params_row), app->quantum_box);
gtk_box_append(GTK_BOX(params_row), app->aging_interval_box);
gtk_box_append(GTK_BOX(params_row), app->max_priority_box);

// Append the horizontal row to the card instead of the individual boxes
gtk_box_append(GTK_BOX(card), params_row);

    
    // Connect algorithm selection signal
    g_signal_connect(app->algo_dropdown, "notify::selected", G_CALLBACK(on_algorithm_selected), app);

    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Add Algorithm Button
    GtkWidget *btn_add_algo = gtk_button_new_with_label("Import Custom Algorithm");
    g_signal_connect(btn_add_algo, "clicked", G_CALLBACK(on_add_algorithm_clicked), app);
    gtk_box_append(GTK_BOX(card), btn_add_algo);

    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Configuration Section
    GtkWidget *config_label = gtk_label_new("Configuration");
    gtk_widget_set_halign(config_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(config_label, "section-label");
    gtk_box_append(GTK_BOX(card), config_label);

    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    app->config_entry = gtk_entry_new();
    if (app->config_filename[0] != '\0'){
        gtk_entry_set_placeholder_text(GTK_ENTRY(app->config_entry), app->config_filename);
    }else{
        gtk_entry_set_placeholder_text(GTK_ENTRY(app->config_entry), "Type filename (e.g. data.txt)...");
    }
    gtk_widget_set_hexpand(app->config_entry, TRUE);
    gtk_box_append(GTK_BOX(input_box), app->config_entry);

    GtkWidget *btn_submit = gtk_button_new_with_label("Load");
    g_signal_connect(btn_submit, "clicked", G_CALLBACK(on_submit_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_submit);

    GtkWidget *btn_browse = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(on_browse_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_browse);

    gtk_box_append(GTK_BOX(card), input_box);

    //edit config
    app->edit_config_btn = gtk_button_new_with_label("Edit Config");
    gtk_widget_add_css_class(app->edit_config_btn, "edit-config-button");
    gtk_widget_set_sensitive(app->edit_config_btn, FALSE);
    g_signal_connect(app->edit_config_btn, "clicked",
                    G_CALLBACK(on_edit_config_clicked), app);

    gtk_box_append(GTK_BOX(card), app->edit_config_btn);

    // Process List
    GtkWidget *list_scroller = gtk_scrolled_window_new();
    gtk_widget_set_size_request(list_scroller, -1, 300);
    gtk_widget_add_css_class(list_scroller, "process-list-container");

    app->process_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->process_list_box), GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroller), app->process_list_box);
    gtk_box_append(GTK_BOX(card), list_scroller);

    // Start Button
    GtkWidget *start_btn = gtk_button_new_with_label("Start Scheduler");
    gtk_widget_add_css_class(start_btn, "start-button");
    gtk_widget_set_halign(start_btn, GTK_ALIGN_FILL);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_clicked), app);
    gtk_box_append(GTK_BOX(card), start_btn);

    // Show Logfile Button
    app->show_logfile_btn = gtk_button_new_with_label("Show Logfile");
    gtk_widget_add_css_class(app->show_logfile_btn, "show-logfile-button");
    gtk_widget_set_halign(app->show_logfile_btn, GTK_ALIGN_FILL);
    gtk_widget_set_sensitive(app->show_logfile_btn, FALSE);
    g_signal_connect(app->show_logfile_btn, "clicked", G_CALLBACK(on_logfile_clicked), app);
    gtk_box_append(GTK_BOX(card), app->show_logfile_btn);

    // Add left card to horizontal container
    gtk_box_append(GTK_BOX(horizontal_container), card);

    // Right card (Gantt Chart) - 3/4 width
    GtkWidget *gantt_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_hexpand(gantt_card, TRUE);
    gtk_widget_add_css_class(gantt_card, "card");

    GtkWidget *gantt_label = gtk_label_new("Execution Timeline");
    gtk_widget_set_halign(gantt_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(gantt_label, "section-label");
    gtk_box_append(GTK_BOX(gantt_card), gantt_label);

    // SCROLLABLE GANTT SECTION
    GtkWidget *gantt_scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gantt_scroller),
                                    GTK_POLICY_AUTOMATIC,  
                                    GTK_POLICY_AUTOMATIC); 
    gtk_widget_set_vexpand(gantt_scroller, TRUE);
    gtk_widget_add_css_class(gantt_scroller, "gantt-container");

    app->gantt_widget = create_gantt_chart_widget();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(gantt_scroller), app->gantt_widget);
    gtk_box_append(GTK_BOX(gantt_card), gantt_scroller);

    // Add right card to horizontal container
    gtk_box_append(GTK_BOX(horizontal_container), gantt_card);



    // Add horizontal container to main container
    gtk_box_append(GTK_BOX(main_container), horizontal_container);

    gtk_window_set_child(GTK_WINDOW(app->window), main_container);

    // CSS Styling - UPDATED gantt-container height for I/O support
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background: linear-gradient(135deg, #000 0%, #764ba2 100%); }"
        ".header-section { color: white; }"
        ".title-label { font-size: 36px; font-weight: bold; }"
        ".card { background: #fff; border-radius: 12px; padding: 20px; }"
        ".section-label { font-weight: bold; margin-bottom: 5px; color: #444; }"
        ".quantum-label { font-size: 14px; color: #666; }"
        ".quantum-input { padding: 8px; border: 1px solid #ddd; border-radius: 4px; }"
        ".process-list-container { margin-top: 15px; border: 1px solid #ddd; border-radius: 5px; background: #f9f9f9; }"
        ".gantt-container { margin-top: 15px; border: 1px solid #ddd; border-radius: 5px; background: white; min-height: 320px; }"
        ".start-button { font-size: 18px; padding: 10px; margin-top: 10px; }"
        ".show-logfile-button { font-size: 18px; padding: 10px; margin-top: 10px; }"
        ".process-row { padding: 10px; border-bottom: 1px solid #eee; }"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);

    gtk_window_present(GTK_WINDOW(app->window));


    if (app->config_filename[0] != '\0'){
        handle_config_submission(app, app->config_filename);
    }else{
        app->config_filename[0] = '\0';
    }
}

static void on_delete_process_clicked(GtkButton *button, gpointer user_data)
{
    AppContext *app = (AppContext *)user_data;
    if (!app || !app->CFG) return;
    if (app->selected_process < 0 || app->selected_process >= app->CFG->process_count) return;

    int idx = app->selected_process;

    /* Shift processes left to overwrite the deleted one */
    for (int i = idx; i < app->CFG->process_count - 1; i++) {
        app->CFG->processes[i] = app->CFG->processes[i + 1];
    }
    app->CFG->process_count--;

    /* Adjust selection */
    if (app->CFG->process_count == 0) {
        app->selected_process = -1;
        gtk_editable_set_text(GTK_EDITABLE(app->id_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(app->arrival_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(app->exec_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(app->priority_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(app->io_entry), "");
    } else {
        if (app->selected_process >= app->CFG->process_count)
            app->selected_process = app->CFG->process_count - 1;
    }

    /* Update editor list */
    refresh_process_list(app);
    if (app->editor_process_list_box && app->selected_process >= 0) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->editor_process_list_box), app->selected_process);
        if (row) gtk_list_box_select_row(GTK_LIST_BOX(app->editor_process_list_box), row);
    }

    /* Persist and reload main UI if a config file is loaded */
    if (app->config_filename && app->config_filename[0] != '\0') {
        char full_path[512];
        if (strstr(app->config_filename, "/") == NULL) {
            snprintf(full_path, sizeof(full_path), "%s/%s", CONFIG_DIR, app->config_filename);
        } else {
            snprintf(full_path, sizeof(full_path), "%s", app->config_filename);
        }

        if (save_config(full_path, app->CFG)) {
            g_print("Config saved to %s\n", full_path);
            handle_config_submission(app, app->config_filename);
        } else {
            g_print("Failed to save config to %s\n", full_path);
        }
    }
}