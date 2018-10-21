#include <stdio.h>
#include <stdlib.h>
#include <game.h>

#define EXPECTED_ARGC 2

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
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: gopher port\n");
            break;
        case CONNECT_ERR:
            fprintf(stderr, "Failed to connect\n");
            break;
        case INVALID_SERVER:
            fprintf(stderr, "Invalid server\n");
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    if (argc != EXPECTED_ARGC) {
        exit_with_error(INVALID_ARG_NUM);
    }
}

int main(int argc, char **argv) {
    check_args(argc, argv);
}