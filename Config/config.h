#ifndef CONFIG_H

#define CONFIG_H

#include "types.h"

typedef struct 
{
    PROCESS processes[20];
    int process_count;
} Config;

void trim(char* str);
int load_config( char* filename, Config* cfg);

#endif // CONFIG_H