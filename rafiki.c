#include "shared.h"

enum Error {
    INVALID_ARG_NUM = 1,
    INVALID_KEYFILE = 2,
    INVALID_DECKFILE = 3,
    INVALID_STATFILE = 4,
    BAD_TIMEOUT = 5,
    FAILED_LISTEN = 6,
    SYSTEM_ERR = 10
};

typedef struct {
    int timeout;
} Server;

void exit_with_error(int error) {
    switch(error) {
        case 1:
            fprintf(stderr, "Usage: rafiki keyfile deckfile statfile timeout\n");
            break;
        case 2:
            fprintf(stderr, "Bad keyfile\n");
            break;
        case 3:
            fprintf(stderr, "Bad deckfile\n");
            break;
        case 4:
            fprintf(stderr, "Bad statfile\n");
            break;
        case 5:
            fprintf(stderr, "Bad timeout\n");
            break;
        case 6:
            fprintf(stderr, "Failed listen\n");
            break;
        case 10:
            fprintf(stderr, "System error\n");
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    
}

void load_deckfile(char *path) {
    
}

void load_statfile(char *path) {
    
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    load_keyfile(argv[1]);
    load_deckfile(argv[2]);
    load_statfile(argv[3]);
}