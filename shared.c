#include "shared.h"

void load_keyfile(char *path) {
    
}

void send_message(Connection *connection, char *message, ...) {
    va_list args;
    va_start(args, message);
    vfprintf(connection->in, message, args);
    va_end(args);
    fflush(connection->in);
}