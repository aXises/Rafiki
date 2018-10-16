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

enum Argument {
    KEYFILE = 1,
    PORT = 2,
    GAME_NAME = 3,
    PLAYER_NAME = 4
};

typedef struct {
    int socket;
    char *rid;
    char *port;
    char *name;
    char *key;
    FILE *in;
    FILE *out;
} Server;

// typedef struct {
//     struct Game game;
// } GameInfo;

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

void listen_server(FILE *out, char **output) {
    read_line(out, output, 0);
    if (strcmp(*output, "eog") == 0) {
        printf("EOG RECIEVED\n");
        exit(0);
    }
}

void get_game_info(Server *server, struct Game *game, struct Player *player) {
    char *buffer;
    listen_server(server->out, &buffer);
    if (strstr(buffer, "rid") != NULL) {
        // verifiy rid here.
        char **splitString = split(buffer, "d");
        server->rid = malloc(sizeof(char) * (strlen(splitString[1]) + 1));
        strcpy(server->rid, splitString[1]);
        server->rid[strlen(splitString[1])] = '\0';
        free(splitString);
    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "playinfo") != NULL) {
        // verifiy playinfo here.
        char **splitString = split(buffer, "o");
        char **playInfo = split(splitString[1], "/");
        initialize_player(player, atoi(playInfo[0]));
        game->playerCount = atoi(playInfo[1]);
        free(playInfo);
        free(splitString);

    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "tokens") != NULL) {
        int output;
        parse_tokens_message(&output, buffer);
        if (output == -1) {
            exit_with_error(COMM_ERR, "");
        }
        for (int i = 0; i < (TOKEN_MAX - 1); i++) {
            game->tokenCount[i] = output;
        }
    }
    free(buffer);
}

struct Game initialize_game(char *name) {
    struct Game game;
    game.name = name;
    return game;
}

void connect_server(Server *server, char *gamename, char *playername) {
    get_socket(server);
    server->in = fdopen(server->socket, "w");
    server->out = fdopen(server->socket, "r");
    send_message(server->in, "12345\n");
    char *buffer;
    read_line(server->out, &buffer, 0);
    printf("recieved from server: %s\n", buffer);
    if (strcmp(buffer, "yes") != 0) {
        return;
    }
    send_message(server->in, "%s\n", gamename);
    send_message(server->in, "%s\n", playername);
    struct Game game = initialize_game(gamename);
    struct Player player;
    get_game_info(server, &game, &player);
    fclose(server->in);
    fclose(server->out);
}

void setup_server(Server *server, char *port, char *keyfile) {
    server->port = port;
    server->key = "12345";
}

void free_server(Server server) {

}

int main(int argc, char **argv) {
    check_args(argc, argv);
    load_keyfile(argv[1]);
    Server server;
    setup_server(&server, argv[PORT], argv[KEYFILE]);
    connect_server(&server, argv[GAME_NAME], argv[PLAYER_NAME]);
    free_server(server);
}