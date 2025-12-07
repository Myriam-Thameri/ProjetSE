#include <dirent.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"


void get_all_files_in_directory(char *dir_path, char files[][20], int *count) {
    struct dirent *de;
    DIR *directory = opendir(dir_path);
    *count = 0;

    if (directory == NULL) {    
        printf("ERROR : Could not open current directory" );
        return ;
    }

    int expreg;
    regex_t regex;
    expreg = regcomp(&regex, "^.+\\.txt$", REG_EXTENDED|REG_ICASE);
    if (expreg !=0)
    {
        printf("ERROR : Could not compile regex\n");
        return ;
    }
    
    while ((de = readdir(directory)) != NULL){
            
            if (regexec(&regex, de->d_name, 0, NULL, 0) == 0){
                printf("%s\n", de->d_name);
                strncpy(files[*count], de->d_name,19);
                files[*count][19] = '\0'; 
                (*count)++;
            }
            
    }
    closedir(directory); 
}