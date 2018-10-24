#include <player.h>
#include "shared.h"

#define EXPECTED_ARGC 5

enum Argument {
    KEYFILE = 1,
    PORT = 2,
    GAME_NAME = 3,
    RECONNECT_ID = 3,
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
} Server;

void exit_with_error(int error, char playerLetter) {
    switch(error) {
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: zazu keyfile port game pname\n");
            break;
        case INVALID_KEYFILE:
            fprintf(stderr, "Bad keyfile\n");
            break;
        case CONNECT_ERR:
            fprintf(stderr, "Failed to connect\n");
            break;
        case BAD_AUTH:
            fprintf(stderr, "Bad auth\n");
            break;
        case BAD_RID:
            fprintf(stderr, "Bad reconnect id\n");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communication Error\n");
            break;
        case PLAYER_DISCONNECTED:
            fprintf(stderr, "Player %c disconnected\n", playerLetter);
            break;
        case INVALID_MESSAGE:
            fprintf(stderr, "Player %c sent invalid message\n", playerLetter);
            break;
    }
    exit(error);
}

void check_args(int argc, char **argv) {
    if (argc != EXPECTED_ARGC || !is_string_digit(argv[PORT])) {
        exit_with_error(INVALID_ARG_NUM, ' ');
    }
    if (atoi(argv[PORT]) < 0 || atoi(argv[PORT]) > 65535) {
        exit_with_error(INVALID_ARG_NUM, ' ');
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
        return CONNECT_ERR;
    }
    *output = sock;
    freeaddrinfo(res0);
    return NOTHING_WRONG;
}

void listen_server(FILE *out, char **output) {
    read_line(out, output, 0);
    if (*output == NULL) {
        return;
    }
    if (strcmp(*output, "eog") == 0) {
        exit(NOTHING_WRONG);
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
        printf("%s\n", buffer);
        char **splitString = split(buffer, "d");
        server->rid = malloc(sizeof(char) * (strlen(splitString[RIGHT]) + 1));
        strcpy(server->rid, splitString[RIGHT]);
        server->rid[strlen(splitString[RIGHT])] = '\0';
        free(splitString);
        free(buffer);
    } else {
        free(buffer);
        return COMM_ERR;
    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "playinfo") != NULL) {
        // verifiy playinfo here.
        printf("%s\n", buffer);
        char **splitString = split(buffer, "o");
        char **playInfo = split(splitString[RIGHT], "/");
        server->game.selfId = atoi(playInfo[LEFT]);
        server->game.playerCount = atoi(playInfo[RIGHT]);
        free(playInfo);
        free(splitString);
        free(buffer);
    } else {
        free(buffer);
        return COMM_ERR;
    }
    listen_server(server->out, &buffer);
    if (strstr(buffer, "tokens") != NULL) {
        printf("%s\n", buffer);
        int output;
        parse_tokens_message(&output, buffer);
        if (output == -1) {
            return INVALID_MESSAGE;
        }
        for (int i = 0; i < (TOKEN_MAX - 1); i++) {
            server->game.tokenCount[i] = output;
        }
        free(buffer);
    } else {
        free(buffer);
        return COMM_ERR;
    }
    return NOTHING_WRONG;
}

enum Error connect_server(Server *server, char *gamename, char *playername) {
    enum Error err = get_socket(&server->socket, server->port);
    if (err) {
        return err;
    }
    server->in = fdopen(server->socket, "w");
    server->out = fdopen(server->socket, "r");
    send_message(server->in, "play%s\n", server->key);
    char *buffer;
    read_line(server->out, &buffer, 0);
    if (strcmp(buffer, "yes") != 0) {
        free(buffer);
        return BAD_AUTH;
    }
    send_message(server->in, "%s\n", gamename);
    send_message(server->in, "%s\n", playername);
    server->gameName = gamename;
    free(buffer);
    return NOTHING_WRONG;
}

void free_server(Server server) {
    free(server.game.players);
    free(server.rid);
    free(server.key);
    fclose(server.in);
    fclose(server.out);
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
        if (state->players[state->selfId].tokens[i] == 0) {
            message.costSpent[i] = 0;
            continue;
        }
        int validToken = 0;
        while(!validToken) {
            printf("Token-%c> ", print_token(i));
            char *tokenTaken;
            read_line(stdin, &tokenTaken, 0);
            if (is_string_digit(tokenTaken) && atoi(tokenTaken) >= 0 &&
                    atoi(tokenTaken) <=
                    state->players[state->selfId].tokens[i]) {
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
            if (strcmp(tokenTaken, "") != 0 && is_string_digit(tokenTaken) &&
                    atoi(tokenTaken) >= 0 &&
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

enum Error handle_messages(Server *server, enum MessageFromHub type,
        char *line) {
    enum ErrorCode err = 0;
    int id;
    switch (type) {
        case END_OF_GAME:
            display_eog_info(&server->game);
            exit_with_error(err, ' ');
        case DO_WHAT:
            printf("dowhat\n");
            make_move(server, &server->game);
            break;
        case PURCHASED:
            printf("%s\n", line);
            err = handle_purchased_message(&server->game, line);
            break;
        case TOOK:
            printf("%s\n", line);
            err = handle_took_message(&server->game, line);
            break;
        case TOOK_WILD:
            printf("%s\n", line);
            err = handle_took_wild_message(&server->game, line);
            break;
        case NEW_CARD:
            printf("%s\n", line);
            err = handle_new_card_message(&server->game, line);
            break;
        case DISCO:
            err = parse_disco_message(&id, line);
            if (err) {
                err = COMM_ERR;
            } else {
                free(line);
                exit_with_error(PLAYER_DISCONNECTED, id + 'A');
            }
        case INVALID:
            err = parse_invalid_message(&id, line);
            if (err) {
                err = COMM_ERR;
            } else {
                free(line);
                exit_with_error(INVALID_MESSAGE, id + 'A');
            }
        default:
            err = COMM_ERR;
    }
    return err;
}

/* Play the game, starting at the first round. Will return at the end of the
 * game, either with 0 or with the relevant exit code.
 */
enum Error play_game(Server *server) {
    enum ErrorCode err = 0;
    while (1) {
        char* line;
        int readBytes = read_line(server->out, &line, 0);
        // printf("recieved from server: %s\n", line);
        if (readBytes <= 0) {
            free(line);
            return COMM_ERR;
        }
        enum MessageFromHub type = classify_from_hub(line);
        err = handle_messages(server, type, line);
        free(line);
        if (err) {
            return err;
        } else if (type != DO_WHAT) {
            // display_turn_info(&server->game);
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
    Server server;
    server.port = argv[PORT];
    server.game.boardSize = 0;
    enum Error err;
    err = load_keyfile(&server.key,  argv[KEYFILE]);
    if (err) {
        exit_with_error(err, ' ');
    }
    err = connect_server(&server, argv[GAME_NAME], argv[PLAYER_NAME]);
    if (err) {
        exit_with_error(err, ' ');
    }
    err = get_game_info(&server);
    if (err) {
        exit_with_error(err, ' ');
    }
    setup_players(&server, server.game.playerCount);
    play_game(&server);
    free_server(server);
}