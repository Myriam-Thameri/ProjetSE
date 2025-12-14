#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

char current_log_path[512];
FILE *log_file = NULL;


void remove_extension(char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot != NULL) {
        *dot = '\0'; 
    }
}

int init_log(const char *algo_name,  const char *config_file) {
    FILE *log_file = NULL;

    #ifdef _WIN32
        mkdir("output");
    #else
        mkdir("output", 0755);
    #endif
    
    remove_extension(config_file);

    snprintf(current_log_path, sizeof(current_log_path), "output/%s_%s.log", algo_name, config_file);

    log_file = fopen(current_log_path, "w");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Could not create log file %s\n", current_log_path);
        return -1;
    }
    
    time_t now = time(NULL);
    fprintf(log_file, "**********************************************************\n");
    fprintf(log_file, "Creation Time: %s", ctime(&now));
    fprintf(log_file, "**********************************************************\n");
    fprintf(log_file, "Algorithm: %s\n", algo_name);
    fprintf(log_file, "Config File Used: %s\n", strcat(config_file,".txt"));
    fprintf(log_file, "**********************************************************\n\n");
    fflush(log_file);
    
    printf("Log file created: %s\n\n", current_log_path);
    return 0;
}

void close_log() {
    if (log_file != NULL) {
        fprintf(log_file, "\n**********************************************************\n");
        fprintf(log_file, "Execution completed\n");
        fprintf(log_file, "**********************************************************\n\n\n");
        fclose(log_file);
        log_file = NULL;
    }
}

int log_print(const char *format, ...) {
    va_list args;

    log_file = fopen(current_log_path, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Could not create log file %s\n", current_log_path);
        return -1;
    }
    
    va_start(args, format);
    vfprintf(log_file, format, args);
    fflush(log_file);  
    va_end(args);
    return  0;
    
}

