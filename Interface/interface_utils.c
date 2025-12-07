#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./interface_utils.h"
#include "../Utils/utils.h"
#include "../Config/config.h"
#include "../Config/types.h"

#define MAX_FILES 50
#define MAX_FILENAME_LEN 20
#define CONFIG_PATH_MAX 256

// DIR_PATH declared as extern to avoid multiple definition - defined in main.c
extern char* DIR_PATH;

// Helper struct for browse dialog
typedef struct {
    SchedulerData *app_data;
    GtkWidget *dialog;
} BrowseDialogData;

void populate_process_list(GtkWidget *list_box, char *filename, Config *CFG) {
    printf("Populating process list for file: %s\n", filename);
    if (!filename || !CFG || !list_box) {
        g_print("Error: Invalid filename, config, or list_box\n");
        return;
    }
    
    int config_load_res;
    char config_file_path[CONFIG_PATH_MAX];
    snprintf(config_file_path, sizeof(config_file_path), "%s/%s", "./Config", filename);
    printf("Using config file: %s\n", config_file_path);
    config_load_res = load_config(config_file_path, CFG);
    

    if (config_load_res != 1) {
        g_print("Error: Failed to load config file\n");
        return;
    }    // 1. Clear existing list items - GTK4 approach for GtkListBox
    if (CFG->process_count < 0 || CFG->process_count > 20) {
        g_print("Error: Invalid process count %d\n", CFG->process_count);
        return;
    }
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(list_box))) != NULL) {
        gtk_list_box_remove(GTK_LIST_BOX(list_box), child);
    }

    // 2. Add status label
    gchar *status_label_text = g_strdup_printf("File selected: %s (%d processes)", filename, CFG->process_count);
    GtkWidget *status_label = gtk_label_new(status_label_text);
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    g_free(status_label_text);
    gtk_list_box_insert(GTK_LIST_BOX(list_box), status_label, -1);

    // 3. Add process entries
    for(int i = 0; i < CFG->process_count; i++){
        char buffer[2048] = {0};
        char temp[256] = {0};
        PROCESS p = CFG->processes[i];
        
        // Use safer string building to prevent buffer overflow
        int offset = 0;
        int remaining = sizeof(buffer) - 1;
        
        offset += snprintf(buffer + offset, remaining, "Process ID: %s", p.ID);
        remaining = sizeof(buffer) - 1 - offset;
        
        if (remaining > 0) {
            offset += snprintf(buffer + offset, remaining, " | Arrival: %d | Exec: %d | Priority: %d", 
                         p.arrival_time, p.execution_time, p.priority);
            remaining = sizeof(buffer) - 1 - offset;
        }
        
        if (p.io_count > 0 && remaining > 0) {
            offset += snprintf(buffer + offset, remaining, " | IO Ops: %d", p.io_count);
            remaining = sizeof(buffer) - 1 - offset;
        }
    
        GtkWidget *row_label = gtk_label_new(buffer);
        gtk_label_set_wrap(GTK_LABEL(row_label), TRUE);
        gtk_widget_set_halign(row_label, GTK_ALIGN_START);
        gtk_list_box_insert(GTK_LIST_BOX(list_box), row_label, -1);
    }
}

// --- Callback for when a file is selected from the custom browse list ---
static void on_custom_file_selected(GtkListBox *list_box, GtkListBoxRow *row, gpointer data) {
    SchedulerData *app_data = (SchedulerData *)data;
    
    if (!row || !app_data) return;
    
    // Get the label widget inside the selected row
    GtkWidget *child_label = gtk_list_box_row_get_child(row);
    if (!child_label) return;
    
    // Get the text (which is the file name)
    const char *filename = gtk_label_get_text(GTK_LABEL(child_label));
    
    if (filename && strlen(filename) > 0) {
        // 1. Update the main entry field
        gtk_editable_set_text(GTK_EDITABLE(app_data->entry), filename);
        
        // 2. Populate the process list box
        populate_process_list(app_data->list_box, filename, app_data->CFG);
        
        // 3. Close the pop-up window
        GtkWidget *dialog = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(list_box)));
        if (GTK_IS_WINDOW(dialog)) {
            gtk_window_close(GTK_WINDOW(dialog));
        }
    }
}


static void on_browse_clicked(GtkWidget *button, gpointer data) {
    SchedulerData *app_data = (SchedulerData *)data;
    
    if (!app_data) return;

    // 1. Create a new dialog/pop-up window
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Select Configuration File",
        GTK_WINDOW(app_data->window), // Parent window
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 400);

    // Get the content area of the dialog
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    // 2. Create a GtkListBox to hold the file names
    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);
    
    // Connect the selection signal to our handler
    g_signal_connect(list_box, "row-activated", G_CALLBACK(on_custom_file_selected), app_data);

    // 3. Populate the list box with the available file names
    for (int i = 0; i < app_data->files_count; i++) {
        GtkWidget *row_label = gtk_label_new(app_data->available_files[i]);
        // Set alignment for the label
        gtk_widget_set_halign(row_label, GTK_ALIGN_START); 
        // Add the label (file name) to the list box
        gtk_list_box_insert(GTK_LIST_BOX(list_box), row_label, -1);
    }

    // 4. Add the list box to a scrolled window for scrollability
    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), list_box);
    gtk_box_append(GTK_BOX(content_area), scrolled_window);

    // 5. Show the dialog and let it run modally
    gtk_widget_show(dialog);
}


// --- Callback for the "Submit" button ---
static void on_submit_clicked(GtkWidget *button, gpointer data) {
    SchedulerData *app_data = (SchedulerData *)data;
    
    if (!app_data) return "";
    
    // Get the text from the GtkEntry widget
    const char *filename = gtk_editable_get_text(GTK_EDITABLE(app_data->entry));

    if (filename != NULL && strlen(filename) > 0) {
        g_print("Submitted filename: %s\n", filename);
        
        // Populate the process list based on the text field content
        populate_process_list(app_data->list_box, filename, app_data->CFG);
        return g_strdup(filename);

    } else {
        g_print("Error: Please enter a file name or use the browse button.\n");
        return "";
    }
}


// --- Main GTK application setup function ---
void activate(GtkApplication *app, gpointer CFG) {
    
    if (!CFG) {
        g_print("Error: Config is NULL\n");
        return;
    }
    
    // 1. Initialize the application data structure
    SchedulerData *app_data = g_new(SchedulerData, 1);
    app_data->CFG = CFG;
    
    // Load your custom file list here (part of initialization)
    get_all_files_in_directory(DIR_PATH, app_data->available_files, &app_data->files_count);
    
    // ... widget creation (same as before) ...
    GtkWidget *window;
    GtkWidget *vbox, *hbox;
    GtkWidget *submit_button, *browse_button;
    GtkWidget *scrolled_window;
    
    // ... (Window and main container setup) ...
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Task Scheduler Interface");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);

    app_data->window = window;
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    // HBox for Input and Buttons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 
    gtk_box_append(GTK_BOX(vbox), hbox);

    // Input Field (GtkEntry)
    app_data->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_data->entry), "Enter file name or select from Browse list...");
    gtk_box_append(GTK_BOX(hbox), app_data->entry);
    gtk_widget_set_hexpand(app_data->entry, TRUE); 

    // "Submit" Button
    submit_button = gtk_button_new_with_label("Submit");
    gtk_box_append(GTK_BOX(hbox), submit_button);
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_clicked), app_data);

    // "Browse" Button
    browse_button = gtk_button_new_with_label("Browse");
    gtk_box_append(GTK_BOX(hbox), browse_button);
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked), app_data);

    // List Box (GtkListBox) for processes
    scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_append(GTK_BOX(vbox), scrolled_window);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    
    app_data->list_box = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), app_data->list_box);
    
    GtkWidget *header_label = gtk_label_new("--- Processes/Tasks to Schedule ---");
    gtk_box_prepend(GTK_BOX(vbox), header_label);

    // Show all widgets
    gtk_widget_show(window);
    
    // Debug: print loaded files
    g_print("Loaded %d config files\n", app_data->files_count);
    for (int i = 0; i < app_data->files_count; i++) {
        g_print("  File %d: %s\n", i + 1, app_data->available_files[i]);
    }
}