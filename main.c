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
    app_data->quantum = 2; // Initialize quantum with default value

    GtkApplication *app = gtk_application_new("com.example.OSScheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    g_free(app_data->CFG);
    g_free(app_data);
    g_object_unref(app);
    return status;
}