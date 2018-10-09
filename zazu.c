#include "shared.h"

enum Error {
    INVALID_ARG_NUM = 1,
    INVALID_KEYFILE = 2,
    CONNECT_ERR = 5,
    FAILED_LISTEN = 6,
    BAD_RID = 7,
    COMM_ERR = 8,
    PLAYER_DISCONNECTED = 9,
    SYSTEM_ERR = 10
};

void exit_with_error(int error, char *playerName) {
    switch(error) {
        case 1:
            fprintf(stderr, "Usage: zazu keyfile port game pname\n");
            break;
        case 2:
            fprintf(stderr, "Bad keyfile\n");
            break;
        case 5:
            fprintf(stderr, "Bad timeout\n");
            break;
        case 6:
            fprintf(stderr, "Failed listen\n");
            break;
        case 7:
            fprintf(stderr, "Bad reconnect id\n");
            break;
        case 8:
            fprintf(stderr, "Communication Error\n");
            break;
        case 9:
            fprintf(stderr, "Player %s disconnected\n", playerName);
            break;
        case 10:
            fprintf(stderr, "Player %s sent invalid message\n", playerName);
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    
}

void load_statfile(char *path) {
    
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    load_keyfile(argv[1]);
}