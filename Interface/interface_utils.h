#ifndef INTERFACE_UTILS_H

#define INTERFACE_UTILS_H

#include <gtk/gtk.h>
#include "../Config/config.h"

#define MAX_FILES 50
#define MAX_FILENAME_LEN 256

typedef struct {
    GtkWidget *entry;       // The input field for the filename
    GtkWidget *list_box;    // The container to show the list of processes
    GtkWidget *window;      // The main window (needed for the file dialog parent)
    char available_files[MAX_FILES][MAX_FILENAME_LEN];
    int files_count;
    Config *CFG;
} SchedulerData;

void populate_process_list(GtkWidget *list_box, char *filename, Config *CFG);
static void on_browse_clicked(GtkWidget *button, gpointer data);
static void on_custom_file_selected(GtkListBox *list_box, GtkListBoxRow *row, gpointer data);
static void on_submit_clicked(GtkWidget *button, gpointer data);
void activate(GtkApplication *app, gpointer config);


#endif // INTERFACE_UTILS_H