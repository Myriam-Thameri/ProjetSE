#ifndef INTERFACE_UTILS_H

#define INTERFACE_UTILS_H

#include <gtk/gtk.h>
#include "../Config/config.h"

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256


typedef struct {
    GtkWidget *window;
    GtkWidget *process_list_box; 
    GtkWidget *editor_process_list_box;
    GtkWidget *algo_dropdown;
    GtkWidget *config_entry;
    GtkWidget *gantt_widget; 
    GtkWidget *show_logfile_btn;
    Config *CFG;
    GtkWidget *quantum_box;   
    GtkWidget *quantum_entry;
    GtkWidget *aging_interval_box;          
    GtkWidget *aging_interval_entry;        
    GtkWidget *max_priority_box;            
    GtkWidget *max_priority_entry; 
    int quantum;   
    char config_filename[124];
    char log_filename[256];
    char available_files[MAX_FILES][MAX_FILENAME_LEN];
    char config_path[512];
    int files_count;
    GtkWidget *edit_config_btn;
    int selected_process;
    GtkWidget *id_entry;
    GtkWidget *arrival_entry;
    GtkWidget *exec_entry;
    GtkWidget *priority_entry;
    GtkWidget *io_entry;
    // Editor widgets
    GtkWidget *entry_ID;
    GtkWidget *entry_arrival_time;
    GtkWidget *entry_execution_time;
    GtkWidget *entry_priority;
    GtkWidget *entry_io_count;

} AppContext;

void activate(GtkApplication *gtk_app, gpointer user_data);
static void on_start_clicked(GtkButton *button, gpointer user_data);
static void on_add_algorithm_clicked(GtkButton *button, gpointer user_data);
static void on_algorithm_file_added(GObject *source, GAsyncResult *result, gpointer user_data);
static void on_algorithm_selected(GObject *dropdown, GParamSpec *pspec, gpointer user_data);
static gboolean algorithm_requires_quantum(const char *algorithm);
static void on_browse_clicked(GtkWidget *button, gpointer user_data);
static void on_browse_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) ;
void handle_config_submission(AppContext *app, const char *filename);
void update_process_list_ui(AppContext *app);
void on_logfile_clicked(GtkWidget *button, gpointer user_data);
void on_edit_config_clicked(GtkButton *button, gpointer user_data);
static GtkWidget* create_process_list_editor(AppContext *app);
static GtkWidget* create_editor_form(AppContext *app);
static void load_process_into_editor(AppContext *app, int index);
static void on_process_row_selected(GtkListBox *box,
                                    GtkListBoxRow *row,
                                    gpointer user_data);
static void on_apply_process_changes(GtkButton *button,
                                     gpointer user_data);
#endif // INTERFACE_UTILS_H