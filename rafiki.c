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

enum ConnectionState {
    AUTHENCATING = 1,
    READY = 2,
    FAILED = 3
};

enum GameState {
    WAITING = 1,
    PLAYING = 2,
    EXITING = 3
};

typedef struct {
    int socket;
    int port;
    FILE *in;
    FILE *out;
    enum ConnectionState state;
} Connection;

typedef struct {
    char *name;
    pthread_t thread;
    int connectionAmount;
    Connection *connections;    
    enum GameState state;
} Game;

typedef struct {
    int timeout;
    int socket;
    int gameAmount;
    Game *games;
    char *port;
    char *key;
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

void add_connection(Game *game, int socket) {
    Connection connection;
    connection.socket = socket;
    connection.in = fdopen(socket, "w");
    connection.out = fdopen(socket, "r");
    connection.state = AUTHENCATING;
    int size = game->connectionAmount;
    game->connections = realloc(game->connections, sizeof(Connection)
            * (size + 1));
    game->connections[size] = connection;
    game->connectionAmount = size + 1;
    printf("connection added\n");
}

void add_game(Server *server, char *name) {
    Game game;
    game.name = name;
    game.state = WAITING;
    game.connections = malloc(0);
    int size = server->gameAmount;
    server->games = realloc(server->games, sizeof(Game) * (size + 1));
    server->games[size] = game;
    server->gameAmount = size + 1;
    printf("new game added\n");
}

void free_connection(Connection connection) {
    
}

// void remove_connection(Server *server, int index) {
//     GameConnection *newArr = malloc(sizeof(GameConnection)
//             * (server->connectionAmount - 1));
//     for (int i = 0; i < server->connectionAmount; i++) {
//         if (i < index) {
//             newArr[i] = server->connections[i];
//         } else if (i > index) {
//             newArr[i] = server->connections[i - 1];
//         }
//     }
//     free_connection(server->connections[index]);
//     free(server->connections);
//     server->connections = newArr;
// }

// int authenticate(Server *server, char *key) {
//     return strcmp(server->key, key) == 0;
// }

// void setup_new_connection(Server *server, GameConnection connection) {
//     char *buffer;
//     read_line(connection.out, &buffer, 0);
//     printf("from player: %s\n", buffer);
//     // if (!authenticate(server, "")) {
        
//     // }
// }

// int index_of_game_name(Server *server, char *name) {
//     int index = -1;
//     for (int i = 0; i < server->connectionAmount; i++) {
//         if (strcmp(server->connections[i].name, name) == 0) {
//             index = i;
//             break;
//         }
//     }
//     return index;
// }

void handle_new_connection() {
    
}

void start_server(Server *server) {
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    int x = 0;
    while(1) {
        int sock;
        sock = accept(server->socket, (struct sockaddr *) &in, &size);
        if (sock == -1) {
            exit_with_error(FAILED_LISTEN);
        }
        // add_connection(server, sock);
        // for (int i = 0; i < server->connectionAmount; i++) {
        //     switch(server->connections[i].state) {
        //         case(INITIALIZING):
        //             setup_new_connection(server, server->connections[i]);
        //             break;
        //         case(WAITING):
        //             break;
        //         case(PLAYING):
        //             break;
        //         case(EXITING):
        //             break;
        //     }
        // }
        // char *buffer;
        // read_line(server->connections[0].out, &buffer, 0);
        // printf("server socket: %i, %s\n", sock, buffer);
        // fprintf(server->connections[0].in, "testback\n");
        // fflush(server->connections[0].in);
        if (x++ > 10) break;
    }
}

void setup_server(Server *server) {
    server->timeout = 0;
    server->port = "3000";
    server->gameAmount = 0;
    server->games = malloc(0);
    server->key = "12345";
}

// void free_server(Server *server) {
//     for (int i = 0; i < server->connectionAmount; i++) {
//         fclose(server->connections[i].in);
//         fclose(server->connections[i].out);
//     }
//     free(server->connections);
// }

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
    // free_server(&server);
}