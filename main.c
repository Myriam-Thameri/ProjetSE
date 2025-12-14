/*
 * Simulateur d'Ordonnancement de Processus
 * Copyright (c) 2025 Équipe ProjetSE - Université Virtuelle de Tunis
 *
 * Licensed under the MIT License
 * See LICENSE file in the project root for full license information.
 */
#include "./Config/types.h"
#include "./Config/config.h"
#include "./Utils/Algorithms.h"
#include "./Utils/utils.h"
#include "./Interface/interface_utils.h"


char* DIR_PATH = "./Config";

int main(int argc, char **argv) {
    AppContext *app_data = g_new0(AppContext, 1);
    
    app_data->CFG = g_new0(Config, 1);
    app_data->quantum = 2;
    

    if (argc > 1) {

        strncpy(app_data->config_filename, argv[1], sizeof(app_data->config_filename) - 1);
        app_data->config_filename[sizeof(app_data->config_filename) - 1] = '\0';

        char config_file_path[256];
        snprintf(config_file_path, sizeof(config_file_path), "./Config/%s", app_data->config_filename);
        
        if (access(config_file_path, F_OK) != 0) {
            g_warning("Le fichier de configuration '%s' n'existe pas", config_file_path);
            app_data->config_filename[0] = '\0';
        } else {
            g_print("Fichier de configuration chargé : %s\n", app_data->config_filename);
        }
    } else {
        app_data->config_filename[0] = '\0';
    }
    
    GtkApplication *app = gtk_application_new("com.example.OSScheduler", 
                                               G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    
    int status = g_application_run(G_APPLICATION(app), 1, argv);
    
    g_free(app_data->CFG);
    g_free(app_data);
    g_object_unref(app);
    return status;
}