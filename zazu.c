#include "shared.h"

enum Error {
    INVALID_ARG_NUM = 1,
    INVALID_KEYFILE = 2,
    CONNECT_ERR = 5,
    FAILED_LISTEN = 6,
    BAD_RID = 7,
    COMM_ERR = 8,
    PLAYER_DISCONNECTED = 9,
    INVALID_MESSAGE = 10
};

typedef struct {
    FILE *in;
    FILE *out;
} Connection;

typedef struct {
    int socket;
    char *port;
    char *name;
} Server;

void exit_with_error(int error, char *playerName) {
    switch(error) {
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: zazu keyfile port game pname\n");
            break;
        case INVALID_KEYFILE:
            fprintf(stderr, "Bad keyfile\n");
            break;
        case CONNECT_ERR:
            fprintf(stderr, "Bad timeout\n");
            break;
        case FAILED_LISTEN:
            fprintf(stderr, "Failed listen\n");
            break;
        case BAD_RID:
            fprintf(stderr, "Bad reconnect id\n");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communication Error\n");
            break;
        case PLAYER_DISCONNECTED:
            fprintf(stderr, "Player %s disconnected\n", playerName);
            break;
        case INVALID_MESSAGE:
            fprintf(stderr, "Player %s sent invalid message\n", playerName);
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    
}

void load_statfile(char *path) {
    
}

void get_socket(Server *server) {
    struct addrinfo hints, *res, *res0;
    int sock;
    const char *cause = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = getaddrinfo(LOCALHOST, server->port, &hints, &res0);
    if (error) {
        exit_with_error(CONNECT_ERR, server->name);
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
            sock = -1;
            close(sock);
            continue;
        }

        break;  /* okay we got one */
    }
    if (sock == -1) {
        exit_with_error(CONNECT_ERR, server->name);
    }
    printf("cause: %s, socket: %i\n", cause, sock);
    server->socket = sock;
    freeaddrinfo(res0);
}

void connect_server(Server *server) {
    get_socket(server);
    printf("client socket: %i\n", server->socket);
    Connection connection;
    connection.in = fdopen(server->socket, "r");
    connection.out = fdopen(server->socket, "w");
    fprintf(connection.out, "test\n");
    fflush(connection.out);
    char *buffer;
    read_line(connection.in, &buffer, 0);
    printf("recieved from server: %s\n", buffer);
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    load_keyfile(argv[1]);
    Server server;
    server.port = "3000";
    connect_server(&server);
}