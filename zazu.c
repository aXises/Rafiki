#include <player.h>
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
    char *gameName;
    char *key;
    FILE *in;
    FILE *out;
    struct GameState game;
    struct GamePlayer *players;
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
        exit_with_error(CONNECT_ERR, "");
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
        exit_with_error(CONNECT_ERR, "");
    }
    printf("cause: %s, socket: %i\n", cause, sock);
    server->socket = sock;
    freeaddrinfo(res0);
}

void listen_server(FILE *out, char **output) {
    read_line(out, output, 0);
    printf("out : %s\n", *output);
    if (*output == NULL) {
        return;
    }
    if (strcmp(*output, "eog") == 0) {
        printf("EOG RECIEVED\n");
        exit(0);
    }
}

enum Error get_game_info(Server *server) {
    char *buffer;
    listen_server(server->out, &buffer);
    if (buffer == NULL) {
        free(buffer);
        return COMM_ERR;
    }
    if (strstr(buffer, "rid") != NULL) {
        // verifiy rid here.
        char **splitString = split(buffer, "d");
        server->rid = malloc(sizeof(char) * (strlen(splitString[1]) + 1));
        strcpy(server->rid, splitString[1]);
        server->rid[strlen(splitString[1])] = '\0';
        free(splitString);
    } else {
        free(buffer);
        return COMM_ERR;
    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "playinfo") != NULL) {
        // verifiy playinfo here.
        char **splitString = split(buffer, "o");
        char **playInfo = split(splitString[1], "/");
        server->game.selfId = atoi(playInfo[0]);
        server->game.playerCount = atoi(playInfo[1]);
        free(playInfo);
        free(splitString);
    } else {
        free(buffer);
        return COMM_ERR;
    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "tokens") != NULL) {
        int output;
        parse_tokens_message(&output, buffer);
        if (output == -1) {
            return INVALID_MESSAGE;
        }
        for (int i = 0; i < (TOKEN_MAX - 1); i++) {
            server->game.tokenCount[i] = output;
        }
    } else {
        return COMM_ERR;
    }
    free(buffer);
    return NOTHING_WRONG;
}

struct Game initialize_game(char *name) {
    struct Game game;
    game.name = name;
    return game;
}

enum Error connect_server(Server *server, char *gamename, char *playername) {
    get_socket(server);
    server->in = fdopen(server->socket, "w");
    server->out = fdopen(server->socket, "r");
    send_message(server->in, "12345\n");
    char *buffer;
    read_line(server->out, &buffer, 0);
    printf("recieved from server: %s\n", buffer);
    if (strcmp(buffer, "yes") != 0) {
        free(buffer);
        return CONNECT_ERR;
    }
    send_message(server->in, "%s\n", gamename);
    send_message(server->in, "%s\n", playername);
    server->gameName = gamename;
    free(buffer);
    return NOTHING_WRONG;
}

void setup_server(Server *server, char *port, char *keyfile) {
    server->port = port;
    server->key = "12345";
}

void free_server(Server server) {

}

void prompt_purchase(Server *server, struct GameState *state) {
    int validCardNumber = 0;
    struct PurchaseMessage message;
    while(!validCardNumber) {
        printf("Card> ");
        char *number;
        read_line(stdin, &number, 0);
        if (is_string_digit(number) && atoi(number) >= 0 &&
                atoi(number) <= 7) {
            message.cardNumber = atoi(number);
            validCardNumber = 1;
        }
        free(number);
    }
    for (int i = 0; i < TOKEN_MAX; i++) {
        if (state->tokenCount[i] == 0) {
            message.costSpent[i] = 0;
            continue;
        }
        int validToken = 0;
        while(!validToken) {
            printf("Token-%c> ", print_token(i));
            char *tokenTaken;
            read_line(stdin, &tokenTaken, 0);
            if (is_string_digit(tokenTaken) && atoi(tokenTaken) >= 0 &&
                    atoi(tokenTaken) <= state->tokenCount[i]) {
                message.costSpent[i] = atoi(tokenTaken);
                validToken = 1;
            }
            free(tokenTaken);
        }
    }
    char *purchaseMessage = print_purchase_message(message);
    send_message(server->in, purchaseMessage);
    free(purchaseMessage);
}

void prompt_take(Server *server, struct GameState *state) {
    struct TakeMessage message;
    for (int i = 0; i < TOKEN_MAX - 1; i++) {
        int validAction = 0;
        while(!validAction) {
            printf("Token-%c> ", print_token(i));
            char *tokenTaken;
            read_line(stdin, &tokenTaken, 0);
            if (is_string_digit(tokenTaken) && atoi(tokenTaken) >= 0 &&
                    atoi(tokenTaken) <= state->tokenCount[i]) {
                message.tokens[i] = atoi(tokenTaken);
                validAction = 1;
            }
            free(tokenTaken);
        }
    }
    char *takeMessage = print_take_message(message);
    send_message(server->in, takeMessage);
    free(takeMessage);
}

void make_move(Server *server, struct GameState *state) {
    int validInput = 0;
    while(!validInput) {
        printf("Action> ");
        char *buffer;
        read_line(stdin, &buffer, 0);
        if (strcmp(buffer, "purchase") == 0) {
            prompt_purchase(server, state);
            validInput = 1;
        } else if (strcmp(buffer, "take") == 0) {
            prompt_take(server, state);
            validInput = 1;
        } else if (strcmp(buffer, "wild") == 0) {
            send_message(server->in, "wild\n");
            validInput = 1;
        }
        free(buffer);
    }
}

/* Play the game, starting at the first round. Will return at the end of the
 * game, either with 0 or with the relevant exit code.
 */
enum Error play_game(Server *server) {
    enum ErrorCode err = 0;
    while (1) {
        char* line;
        int readBytes = read_line(server->out, &line, 0);
        if (readBytes <= 0) {
            return COMM_ERR;
        }
        enum MessageFromHub type = classify_from_hub(line);
        switch (type) {
            case END_OF_GAME:
                free(line);
                // display_eog_info(server->game);
                return NOTHING_WRONG;
            case DO_WHAT:
                printf("Received dowhat\n");
                make_move(server, &server->game);
                break;
            case PURCHASED:
                err = handle_purchased_message(&server->game, line);
                break;
            case TOOK:
                err = handle_took_message(&server->game, line);
                break;
            case TOOK_WILD:
                err = handle_took_wild_message(&server->game, line);
                break;
            case NEW_CARD:
                err = handle_new_card_message(&server->game, line);
                break;
            default:
                free(line);
                return COMM_ERR;
        }
        free(line);
        if (err) {
            return err;
        }
    }
}

void setup_players(Server *server, int amount) {
    server->game.players = malloc(sizeof(struct GamePlayer) * amount);
    for (int i = 0; i < amount; i++) {
        initialize_player(&server->game.players[i], i);
    }
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    load_keyfile(argv[1]);
    Server server;
    setup_server(&server, argv[PORT], argv[KEYFILE]);
    enum Error err = 0;
    err = connect_server(&server, argv[GAME_NAME], argv[PLAYER_NAME]);
    printf("Connected\n");
    err = get_game_info(&server);
    if (err != 0) {
        exit_with_error(err, "");
    }
    // struct GameState g = server.game;
    // struct Player p = server.players.state;
    // printf("game info: %s %i %i %i\n", server.gameName, g.playerCount, p.playerId, g.tokenCount[0]);
    // Starting playing game
    setup_players(&server, server.game.playerCount);
    play_game(&server);
    free_server(server);
    free(server.players);
    fclose(server.in);
    fclose(server.out);
}