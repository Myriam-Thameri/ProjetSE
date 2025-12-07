#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./Config/types.h"
#include "./Config/config.h"
#include "./Algorithms/Algorithms.h"
#include "./Utils/utils.h"
#include "./Interface/interface_utils.h"

#define MAX_FILES 50
#define MAX_FILENAME_LEN 20


char* DIR_PATH = "./Config";


/* static void on_activate (GtkApplication *app) {
    GtkWidget *window, *box, *file_input, *exit_button, *file_button;
    
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Task Scheduler");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 50);
    gtk_widget_set_margin_end(box, 50);
    gtk_widget_set_margin_top(box, 50);
    gtk_widget_set_margin_bottom(box, 50);
    gtk_window_set_child(GTK_WINDOW(window), box);
    
    file_input = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(file_input), "Enter the config file name...");
    gtk_box_append(GTK_BOX(box), file_input);
    
  
    exit_button = gtk_button_new_with_label ("Exit!");
    // When the button is clicked, close the window passed as an argument
    g_signal_connect_swapped (exit_button, "clicked", G_CALLBACK (gtk_window_close), window);
    gtk_window_set_child (GTK_WINDOW (window), exit_button);
    gtk_window_present (GTK_WINDOW (window));
}
 */
int main(int argc, char *argv[]){
    

    Config* CFG = malloc(sizeof(Config));
    char files[MAX_FILES][MAX_FILENAME_LEN];
    int files_count = 0;
    get_all_files_in_directory(DIR_PATH , files, &files_count);
    int config_load_res;
    printf("Files in directory %s:\n", DIR_PATH);
    for (int i = 0; i < files_count; i++) {
        printf("File %d: %s\n", i + 1, files[i]);
    }
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.TaskScheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), CFG);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    free(CFG);

    return status;
    /* snprintf(CONFIG_FILE_PATH, sizeof(CONFIG_FILE_PATH), "%s/%s", DIR_PATH, "config.txt");
    printf("Using config file: %s\n", CONFIG_FILE_PATH);
    config_load_res = load_config(CONFIG_FILE_PATH , CFG); 
    
    if(config_load_res == 0) {
        printf("Error loading config file.\n");
        return 1;
    }else if (config_load_res == 1) {
        
    }  */
    // 3. START GTK APPLICATION
    // This runs after the user selects '0'
    
}
