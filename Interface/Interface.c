#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Assumed Includes ---
// Ensure these exist in your project
#include "../functionnalities.h" 
#include "../Utils/utils.h"
#include "./interface_utils.h"
#include "../Config/config.h"
#include "../Config/types.h"

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256

// --- Context Structure ---
typedef struct {
    GtkWidget *window;
    GtkWidget *process_list_box; 
    GtkWidget *algo_dropdown;
    GtkWidget *config_entry;      // The text input field
    Config *CFG;
    
    // File browsing data
    char available_files[MAX_FILES][MAX_FILENAME_LEN];
    int files_count;
} AppContext;

// --- Helper: Get Files from Directory ---
// Scans the ./Config directory for files to populate the Browse list
void scan_config_directory(AppContext *app) {
    GDir *dir;
    const gchar *filename;
    app->files_count = 0;

    dir = g_dir_open(CONFIG_DIR, 0, NULL);
    if (dir) {
        while ((filename = g_dir_read_name(dir))) {
            // Filter for .txt, .conf, .cfg if needed
            if (g_str_has_suffix(filename, ".txt") || 
                g_str_has_suffix(filename, ".conf") || 
                g_str_has_suffix(filename, ".cfg")) {
                
                if (app->files_count < MAX_FILES) {
                    strncpy(app->available_files[app->files_count], filename, MAX_FILENAME_LEN - 1);
                    app->files_count++;
                }
            }
        }
        g_dir_close(dir);
    }
}

// --- Helper: UI Update Logic ---
void update_process_list_ui(AppContext *app) {
    GtkWidget *list_box = app->process_list_box;
    Config *CFG = app->CFG;

    // 1. Clear existing list items
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

    // 2. Add status header
    char status_text[128];
    snprintf(status_text, sizeof(status_text), "Loaded %d processes", CFG->process_count);
    GtkWidget *status_label = gtk_label_new(status_text);
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(status_label, "list-header");
    gtk_list_box_append(GTK_LIST_BOX(list_box), status_label);

    // 3. Add process entries
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

// --- Core Logic: Load Config by Name ---
void handle_config_submission(AppContext *app, const char *filename) {
    if (!filename || strlen(filename) == 0) {
        g_print("Error: Empty filename\n");
        return;
    }

    char full_path[512];
    // Check if user already typed the directory
    if (strstr(filename, "/") == NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", CONFIG_DIR, filename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s", filename);
    }

    g_print("Attempting to load: %s\n", full_path);
    
    // Load config
    int res = load_config(full_path, app->CFG);
    
    if (res == 1) {
        g_print("Success.\n");
        update_process_list_ui(app);
    } else {
        g_print("Failed to load config.\n");
        // Clear UI to indicate failure
        app->CFG->process_count = 0;
        update_process_list_ui(app);
    }
}

// --- Callbacks ---

static void on_submit_clicked(GtkWidget *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(app->config_entry));
    handle_config_submission(app, text);
}

// Triggered when a row in the "Browse" popup is clicked
static void on_browse_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    
    GtkWidget *label = gtk_list_box_row_get_child(row);
    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    
    // 1. Set text in main entry
    gtk_editable_set_text(GTK_EDITABLE(app->config_entry), filename);
    
    // 2. Automatically submit/load
    handle_config_submission(app, filename);
    
    // 3. Close the dialog (find parent window)
    GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_native(GTK_WIDGET(box)));
    gtk_window_close(GTK_WINDOW(toplevel));
}

static void on_browse_clicked(GtkWidget *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    
    // Refresh file list
    scan_config_directory(app);

    // Create Modal Dialog
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
    
    // Add a close button at the bottom
    GtkWidget *close_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(content_box), close_btn);

    gtk_window_present(GTK_WINDOW(dialog));
}

// Logic for Algo dropdown
static void on_algorithm_selected(GObject *dropdown, GParamSpec *pspec, gpointer user_data) {
    GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(dropdown)));
    guint index = gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));
    if (index != GTK_INVALID_LIST_POSITION) {
        const char *algorithm = gtk_string_list_get_string(list, index);
        g_print("Selected algorithm: %s\n", algorithm);
    }
}

// Logic for Add Algorithm File
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

static void on_start_clicked(GtkButton *button, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;
    g_print("Start clicked! Processes loaded: %d\n", app->CFG->process_count);
}

// --- Main UI Setup ---
void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppContext *app = (AppContext *)user_data;

    // Load initial algorithms (Mocking the function call)
    int counts = 0;
    char **algorithms = get_algorithms(&counts); 

    GtkStringList *algo_list = gtk_string_list_new(NULL);
    for (int i = 0; i < counts; i++) {
        gtk_string_list_append(algo_list, algorithms[i]);
    }

    // Window Setup
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "OS Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 750);

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

    // Algo Section
    GtkWidget *algo_label = gtk_label_new("Select Scheduling Algorithm");
    gtk_widget_set_halign(algo_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(algo_label, "section-label");
    gtk_box_append(GTK_BOX(card), algo_label);

    app->algo_dropdown = gtk_drop_down_new(G_LIST_MODEL(algo_list), NULL);
    gtk_widget_add_css_class(app->algo_dropdown, "dropdown");
    g_signal_connect(app->algo_dropdown, "notify::selected", G_CALLBACK(on_algorithm_selected), NULL);
    gtk_box_append(GTK_BOX(card), app->algo_dropdown);

    gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // --- NEW CONFIGURATION SECTION (Type & Search) ---
    GtkWidget *config_label = gtk_label_new("Configuration");
    gtk_widget_set_halign(config_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(config_label, "section-label");
    gtk_box_append(GTK_BOX(card), config_label);

    // Box to hold Input + Buttons side by side
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    // 1. Input Field
    app->config_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->config_entry), "Type filename (e.g. data.txt)...");
    gtk_widget_set_hexpand(app->config_entry, TRUE);
    gtk_box_append(GTK_BOX(input_box), app->config_entry);

    // 2. Submit Button
    GtkWidget *btn_submit = gtk_button_new_with_label("Load");
    g_signal_connect(btn_submit, "clicked", G_CALLBACK(on_submit_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_submit);

    // 3. Browse Button
    GtkWidget *btn_browse = gtk_button_new_with_label("Browse...");
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(on_browse_clicked), app);
    gtk_box_append(GTK_BOX(input_box), btn_browse);

    gtk_box_append(GTK_BOX(card), input_box);

    // Process List (Visual Feedback)
    GtkWidget *list_scroller = gtk_scrolled_window_new();
    gtk_widget_set_size_request(list_scroller, -1, 200);
    gtk_widget_add_css_class(list_scroller, "process-list-container");
    
    app->process_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->process_list_box), GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroller), app->process_list_box);
    gtk_box_append(GTK_BOX(card), list_scroller);

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
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_clicked), app);
    gtk_box_append(GTK_BOX(main_container), start_btn);

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
        ".start-button { font-size: 18px; padding: 10px; margin-bottom: 20px; }"
        ".process-row { padding: 10px; border-bottom: 1px solid #eee; }"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), 800);

    gtk_window_present(GTK_WINDOW(app->window));
}

int main(int argc, char **argv) {
    AppContext *app_data = g_new0(AppContext, 1);
    app_data->CFG = g_new0(Config, 1); 

    GtkApplication *app = gtk_application_new("com.example.FixedScheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_free(app_data->CFG);
    g_free(app_data);
    g_object_unref(app);
    return status;
}