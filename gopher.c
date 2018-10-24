#include "shared.h"

#define EXPECTED_ARGC 2

enum Argument {
    PORT = 1
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
    if (!is_string_digit(argv[PORT])) {
        exit_with_error(CONNECT_ERR);
    }
    if (atoi(argv[PORT]) < 0 || atoi(argv[PORT]) > 65535) {
        exit_with_error(CONNECT_ERR);
    }
}

enum Error get_socket(int *output, char *port) {
    struct addrinfo hints, *res, *res0;
    int sock;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = getaddrinfo(LOCALHOST, port, &hints, &res0);
    if (error) {
        freeaddrinfo(res0);
        return CONNECT_ERR;
    }
    sock = -1;
    for (res = res0; res != NULL; res = res->ai_next) {
        sock = socket(res->ai_family, res->ai_socktype,
           res->ai_protocol);
        if (sock == -1) {
            sock = -1;
            continue;
        }

        if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
            close(sock);
            sock = -1;
            continue;
        }

        break;  /* okay we got one */
    }
    if (sock == -1) {
        freeaddrinfo(res0);
        return CONNECT_ERR;
    }
    *output = sock;
    freeaddrinfo(res0);
    return NOTHING_WRONG;
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    int sock;
    enum Error error = get_socket(&sock, argv[PORT]);
    if (error) {
        exit_with_error(CONNECT_ERR);
    }
    FILE *toServer = fdopen(sock, "w");
    FILE *fromServer = fdopen(sock, "r");
    send_message(toServer, "scores\n");
    char *buffer;
    if (feof(fromServer)) {
        fclose(toServer);
        fclose(fromServer);
        exit_with_error(INVALID_SERVER);
    }
    int bytesRead = read_line(fromServer, &buffer, 0);
    if (bytesRead == 0) {
        free(buffer);
        fclose(toServer);
        fclose(fromServer);
        exit_with_error(NORMAL_EXIT);
    }
    if (!(strcmp(buffer, "yes") == 0)) {
        free(buffer);
        fclose(toServer);
        fclose(fromServer);
        exit_with_error(INVALID_SERVER);
    };
    free(buffer);
    char *scores;
    while (1) {
        read_line(fromServer, &scores, 0);
        if (feof(fromServer)) {
            break;
        }
        printf("%s\n", scores);
        free(scores);
    }
    fclose(toServer);
    fclose(fromServer);
}