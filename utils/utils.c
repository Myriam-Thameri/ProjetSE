#include "utils.h"
#include <stdio.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

void get_all_files_in_directory(char *dir_path, char files[][20], int *count); {
    struct dirent *de;
    DIR *directory = opendir(dir_path);
    count = 0;

    if (directory == NULL) {    
        printf("ERROR : Could not open current directory" );
        return 0;
    }

    int expreg;
    regex_t regex;
    expreg = regcomp(&regex, "^.+\\.txt$", REG_EXTENDED|REG_ICASE);
    if (expreg !=0)
    {
        printf("ERROR : Could not compile regex\n");
        return files;
    }
    
    while ((de = readdir(directory)) != NULL){
            printf("%s\n", de->d_name);
            if (regexec(&regex, de->d_name, 0, NULL, 0) == 0){
                strcpy(files[i], de->d_name);
            }
            count++;
    }
    closedir(dr);
    return files;   
}