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
    int socket;
    int port;
    char *hostname;
    char *name;
    pthread_t thread;
    FILE *in;
    FILE *out;
} GameConnection;

typedef struct {
    int timeout;
    int socket;
    int connectionAmount;
    GameConnection *connections;
    char *port;
} Server;

void exit_with_error(int error) {
    switch(error) {
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: rafiki keyfile deckfile statfile timeout\n");
            break;
        case INVALID_KEYFILE:
            fprintf(stderr, "Bad keyfile\n");
            break;
        case INVALID_DECKFILE:
            fprintf(stderr, "Bad deckfile\n");
            break;
        case INVALID_STATFILE:
            fprintf(stderr, "Bad statfile\n");
            break;
        case BAD_TIMEOUT:
            fprintf(stderr, "Bad timeout\n");
            break;
        case FAILED_LISTEN:
            fprintf(stderr, "Failed listen\n");
            break;
        case SYSTEM_ERR:
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

void get_socket(Server *server) {
    struct addrinfo hints, *res, *res0;
    int sock;
    const char *cause = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = getaddrinfo(LOCALHOST, server->port, &hints, &res0);
    if (error) {
        exit_with_error(FAILED_LISTEN);
    }
    sock = -1;
    for (res = res0; res != NULL; res = res->ai_next) {
        sock = socket(res->ai_family, res->ai_socktype,
                res->ai_protocol);
        if (sock == -1) {
            sock = -1;
            continue;
        }
        if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
            sock = -1;
            close(sock);
            continue;
        }
        if (listen(sock, 5) == -1) {
            sock = -1;
            close(sock);
            continue;
        }
        break;  /* okay we got one */
    }
    if (sock == -1) {
        exit_with_error(FAILED_LISTEN);
    }
    printf("cause: %s, socket: %i\n", cause, sock);
    server->socket = sock;
    freeaddrinfo(res0);
}

void add_connection(Server *server, int socket) {
    GameConnection connection;
    connection.socket = socket;
    connection.in = fdopen(socket, "w");
    connection.out = fdopen(socket, "r");
    int size = server->connectionAmount;
    server->connections = realloc(server->connections, sizeof(GameConnection)
            * (size + 1));
    server->connections[size] = connection;
    server->connectionAmount = size + 1;
    printf("connection added\n");
}

void start_server(Server *server) {
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    int x = 0;
    while(1) {
        int sock;
        if (server->connectionAmount < 1) {
            sock = accept(server->socket, (struct sockaddr *) &in, &size);
            if (sock == -1) {
                exit_with_error(FAILED_LISTEN);
            }
            add_connection(server, sock);
        }
        char *buffer;
        read_line(server->connections[0].out, &buffer, 0);
        printf("server socket: %i, %s\n", sock, buffer);
        fprintf(server->connections[0].in, "testback\n");
        fflush(server->connections[0].in);
        if (x++ > 10) break;
    }
}

void setup_server(Server *server) {
    server->timeout = 0;
    server->port = "3000";
    server->connectionAmount = 0;
    server->connections = malloc(0);
}

void free_server(Server *server) {
    for (int i = 0; i < server->connectionAmount; i++) {
        fclose(server->connections[i].in);
        fclose(server->connections[i].out);
    }
    free(server->connections);
}

void signal_handler(int sig) {
    switch(sig) {
        case(SIGINT):
            printf("signint detected\n");
            exit(0); // remove this and kill players instead.
            break;
        case(SIGTERM):
            printf("signterm detected\n");
            exit(0);
            break;
    }
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
}

int main(int argc, char **argv) {
    setup_signal_handler();
    check_args(argc, argv);
    load_keyfile(argv[1]);
    load_deckfile(argv[2]);
    load_statfile(argv[3]);
    Server server;
    setup_server(&server);
    get_socket(&server);
    start_server(&server);
    free_server(&server);
}