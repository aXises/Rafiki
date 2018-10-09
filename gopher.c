#include <stdio.h>
#include <stdlib.h>
#include <game.h>

enum Error {
    INVALID_ARG_NUM = 1,
    CONNECT_ERR = 3,
    INVALID_SERVER = 4,
};

typedef struct {
    int timeout;
} Server;

void exit_with_error(int error) {
    switch(error) {
        case 1:
            fprintf(stderr, "Usage: gopher port\n");
            break;
        case 3:
            fprintf(stderr, "Failed to connect\n");
            break;
        case 4:
            fprintf(stderr, "Invalid server\n");
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    
}

int main(int argc, char **argv) {
    check_args(argc, argv);
}