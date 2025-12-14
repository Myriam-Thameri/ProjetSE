#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>     
#include <dirent.h>    
#include "utils.h"
#include "../Interface/interface_utils.h"
#include "../Config/config.h" 

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256

void scan_config_directory(AppContext *app) {
    GDir *dir;
    const gchar *filename;
    app->files_count = 0;

    dir = g_dir_open(CONFIG_DIR, 0, NULL);
    if (dir) {
        while ((filename = g_dir_read_name(dir))) {
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

char **get_algorithms(int *count) {    
    DIR *dir = opendir("Algorithms/");
    if (!dir) return NULL;

    char **list = malloc(sizeof(char*) * 50); 
    *count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".c") != 0) continue;

        int len = ext - entry->d_name;

        char *name = malloc(len + 1);
        strncpy(name, entry->d_name, len);
        name[len] = '\0';

        list[*count] = name;
        (*count)++;
    }

    closedir(dir);
    return list;
}
