#include "shared.h"

void load_keyfile(char *path) {
    
}

void send_message(FILE *in, char *message, ...) {
    va_list args;
    va_start(args, message);
    vfprintf(in, message, args);
    va_end(args);
    fflush(in);
}