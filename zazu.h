#ifndef ZAZU_H
#define ZAZU_H

#include <player.h>
#include "shared.h"

#define EXPECTED_ARGC 5

/**
 * Enum for zazu arguments.
 */
enum Argument {
    KEYFILE = 1,
    PORT = 2,
    GAME_NAME = 3,
    RECONNECT_ID = 3,
    PLAYER_NAME = 4
};

/**
 * Type defination for the server the current player is connected to.
 */
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

/**
 * Function prototypes
 */
void exit_with_error(int error, char playerLetter);
int is_newline_or_comma(char character);
void check_args(int argc, char **argv);
enum Error get_socket(int *output, char *port);
void listen_server(FILE *out, char **output);
enum Error get_game_info(Server *server);
enum Error connect_server(Server *server, char *gamename, char *playername);
void free_server(Server server);
void prompt_purchase(Server *server, struct GameState *state);
void prompt_take(Server *server, struct GameState *state);
void make_move(Server *server, struct GameState *state);
enum Error handle_messages(Server *server, enum MessageFromHub type,
        char *line);
enum Error play_game(Server *server);
void setup_players(Server *server, int amount);

#endif