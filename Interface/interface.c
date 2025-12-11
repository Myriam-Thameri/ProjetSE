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
        update_process_list_ui(app);
    } else {
        g_print("Failed to load config.\n");
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

static void on_algorithm_selected(GObject *dropdown, GParamSpec *pspec, gpointer user_data) {
    GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(dropdown)));
    guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));
    if (index != GTK_INVALID_LIST_POSITION) {
        const char *algorithm = gtk_string_list_get_string(list, index);
        g_print("Selected algorithm: %s\n", algorithm);
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
    char config_copy[256];
    strncpy(config_copy, app->config_filename, sizeof(config_copy) - 1);
    remove_extension(config_copy);
    snprintf(app->log_filename, sizeof(app->log_filename), "output/%s_%s.log", algorithm, config_copy);

    init_log(algorithm , app->config_filename);
    
    // Clear previous Gantt data before running algorithm
    clear_gantt_slices();
    
    // Call the appropriate scheduling algorithm based on selection
    if (strcmp(algorithm, "fcfs") == 0) {
        FCFS_Algo(app->CFG);
    } 
    else if (strcmp(algorithm, "RoundRobin") == 0) {
        RoundRobin_Algo(app->CFG);
    }
    else if (strcmp(algorithm, "multilevel_aging") == 0) {
        MultilevelAgingScheduler(app->CFG);
    }
    else if (strcmp(algorithm, "PreemptivePriority") == 0) {
        // You might want to add a quantum input field in the UI
        int quantum = 2; // Default quantum
        run_priority_preemptive(app->CFG);
    }
    else if (strcmp(algorithm, "SJF") == 0 ) {
        SJF_Algo(app->CFG);
    }
    else if (strcmp(algorithm, "MultilevelStatic") == 0) {
        MultilevelStaticScheduler(app->CFG);
    }
    else if (strcmp(algorithm, "srt") == 0) {
        SRT_Algo(app->CFG);
    }
    else {
        g_print("Warning: Unknown algorithm '%s'\n", algorithm);
    }
    
    // Refresh the Gantt chart after algorithm completes
    gtk_widget_queue_draw(app->gantt_widget);
    
    g_print("Scheduling complete. Gantt chart updated with %d slices.\n", slice_count);
    
    gtk_widget_set_sensitive(app->show_logfile_btn, TRUE);
}

void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    app->config_filename[0] = '\0';
    int counts = 0;
    char **algorithms = get_algorithms(&counts); 


    GtkStringList *algo_list = gtk_string_list_new(NULL);
    for (int i = 0; i < counts; i++) {
        gtk_string_list_append(algo_list, algorithms[i]);
    }

    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "OS Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 900, 850);

    GtkWidget *main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Header
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(header_box, 30);
    gtk_widget_set_margin_bottom(header_box, 30);
    gtk_widget_set_halign(header_box, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(header_box, "header-section");
    gtk_box_append(GTK_BOX(header_box), gtk_label_new("⚙"));
    GtkWidget *title = gtk_label_new("OS Scheduler");
    gtk_widget_add_css_class(title, "title-label");
    gtk_box_append(GTK_BOX(header_box), title);
    gtk_box_append(GTK_BOX(main_container), header_box);

    // Card
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(card, 50);
    gtk_widget_set_margin_end(card, 50);
    gtk_widget_set_margin_bottom(card, 30);
    gtk_widget_add_css_class(card, "card");

    // Algorithm Section
    GtkWidget *algo_label = gtk_label_new("Select Scheduling Algorithm");
    gtk_widget_set_halign(algo_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(algo_label, "section-label");
    gtk_box_append(GTK_BOX(card), algo_label);

    app->algo_dropdown = gtk_drop_down_new(G_LIST_MODEL(algo_list), NULL);
    gtk_widget_add_css_class(app->algo_dropdown, "dropdown");
    g_signal_connect(app->algo_dropdown, "notify::selected", G_CALLBACK(on_algorithm_selected), NULL);
    gtk_box_append(GTK_BOX(card), app->algo_dropdown);

    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Configuration Section
    GtkWidget *config_label = gtk_label_new("Configuration");
    gtk_widget_set_halign(config_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(config_label, "section-label");
    gtk_box_append(GTK_BOX(card), config_label);

    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    app->config_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->config_entry), "Type filename (e.g. data.txt)...");
    gtk_widget_set_hexpand(app->config_entry, TRUE);
    gtk_box_append(GTK_BOX(input_box), app->config_entry);

    GtkWidget *btn_submit = gtk_button_new_with_label("Load");
    g_signal_connect(btn_submit, "clicked", G_CALLBACK(on_submit_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_submit);

    GtkWidget *btn_browse = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(on_browse_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_browse);

    gtk_box_append(GTK_BOX(card), input_box);

    // Process List
    GtkWidget *list_scroller = gtk_scrolled_window_new();
    gtk_widget_set_size_request(list_scroller, -1, 150);
    gtk_widget_add_css_class(list_scroller, "process-list-container");
    
    app->process_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->process_list_box), GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroller), app->process_list_box);
    gtk_box_append(GTK_BOX(card), list_scroller);

    // NEW: Gantt Chart Section
    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    GtkWidget *gantt_label = gtk_label_new("Execution Timeline");
    gtk_widget_set_halign(gantt_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(gantt_label, "section-label");
    gtk_box_append(GTK_BOX(card), gantt_label);
    
    app->gantt_widget = create_gantt_chart_widget();
    gtk_widget_add_css_class(app->gantt_widget, "gantt-container");
    gtk_box_append(GTK_BOX(card), app->gantt_widget);

    // Add Algorithm Button
    GtkWidget *btn_add_algo = gtk_button_new_with_label("Import Custom Algorithm");
    g_signal_connect(btn_add_algo, "clicked", G_CALLBACK(on_add_algorithm_clicked), app);
    gtk_box_append(GTK_BOX(card), btn_add_algo);

    gtk_box_append(GTK_BOX(main_container), card);

    // Start Button
    GtkWidget *start_btn = gtk_button_new_with_label("Start Scheduler");
    gtk_widget_add_css_class(start_btn, "start-button");
    gtk_widget_set_margin_start(start_btn, 50);
    gtk_widget_set_margin_end(start_btn, 50);
    gtk_widget_set_halign(start_btn, GTK_ALIGN_FILL);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_clicked), app );
    gtk_box_append(GTK_BOX(main_container), start_btn);

    gtk_window_set_child(GTK_WINDOW(app->window), main_container);
// Show Logfile Button (FIXED: Added to main_container, not replacing it)
    app->show_logfile_btn = gtk_button_new_with_label("Show Logfile");
    gtk_widget_add_css_class(app->show_logfile_btn, "show-logfile-button");
    gtk_widget_set_margin_start(app->show_logfile_btn, 50);
    gtk_widget_set_margin_end(app->show_logfile_btn, 50);
    gtk_widget_set_margin_bottom(app->show_logfile_btn, 20);
    gtk_widget_set_halign(app->show_logfile_btn, GTK_ALIGN_FILL);
    gtk_widget_set_sensitive(app->show_logfile_btn, FALSE);  // DISABLED initially
    g_signal_connect(app->show_logfile_btn, "clicked", G_CALLBACK(on_logfile_clicked), app);
    gtk_box_append(GTK_BOX(main_container), app->show_logfile_btn);  // FIXED: append to container

    // NOW set the child (only once!)
    gtk_window_set_child(GTK_WINDOW(app->window), main_container);

    // CSS Styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }"
        ".header-section { color: white; }"
        ".title-label { font-size: 36px; font-weight: bold; }"
        ".card { background: white; border-radius: 12px; padding: 20px; }"
        ".section-label { font-weight: bold; margin-bottom: 5px; color: #444; }"
        ".process-list-container { margin-top: 15px; border: 1px solid #ddd; border-radius: 5px; background: #f9f9f9; }"
        ".gantt-container { margin-top: 15px; border: 1px solid #ddd; border-radius: 5px; background: white; min-height: 150px; }"
        ".start-button { font-size: 18px; padding: 10px; margin-bottom: 20px; }"
        ".process-row { padding: 10px; border-bottom: 1px solid #eee; }"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);

    gtk_window_present(GTK_WINDOW(app->window));
}

