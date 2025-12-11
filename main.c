#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./Config/types.h"
#include "./Config/config.h"
#include "./Utils/Algorithms.h"
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
    
    AppContext *app_data = g_new0(AppContext, 1);
    app_data->CFG = g_new0(Config, 1); 

    GtkApplication *app = gtk_application_new("com.example.OSScheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_free(app_data->CFG);
    g_free(app_data);
    g_object_unref(app);
    return status;
    
}

