#ifndef UTILS_H

#define UTILS_H

#include "../Interface/interface_utils.h"

#define CONFIG_DIR "./Config"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 256

void scan_config_directory(AppContext *app);
char **get_algorithms(int *count);
#endif // UTILS_H