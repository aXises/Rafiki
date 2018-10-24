#ifndef RAFIKI_H
#define RAFIKI_H

#include "shared.h"

#define EXPECTED_STATFILE_SEP 3
#define EXPECTED_ARGC 5

/**
 * Enum for rafiki arguments.
 */
enum Args {
    KEYFILE = 1,
    DECKFILE = 2,
    STATFILE = 3,
    TIMEOUT = 4
};

/**
 * Enum for statfile indexes.
 */
enum StatFile {
    PORT = 0,
    START_TOKENS = 1,
    START_POINTS = 2,
    START_PLAYERS = 3,
};

/**
 * Enum for connection types to the server.
 */
enum ConnectionType {
    PLAYER_CONNECT,
    SCORES_CONNECT,
    INVALID_CONNECT,
};

/**
 * Type defination for a score entry.
 */
typedef struct {
    char *playerName;
    int tokensTaken;
    int pointsEarned;
} ScoreEntry;

/**
 * Type defination for a table of score entries.
 */
typedef struct {
    int entryCount;
    ScoreEntry *entries;
} ScoreTable;

/**
 * Type defination properties of a game also stores instances of games with
 * the properties of the type.
 */
typedef struct {
    int socket;
    char *port;
    char *key;
    pthread_t mainThread;
    int playerMax;
    int currentGameIndex;
    int instanceSize;
    struct Game *instances;
    pthread_t *instanceThreads;
    int startToken;
    int winPoints;
    int timeout;
    ScoreTable scoresTable;
} GameProp;

/**
 * Type defination for the main server.
 */
typedef struct {
    int timeout;
    int socket;
    int portAmount;
    GameProp *gameProps;
    char *key;
    char **ports;
    int deckSize;
    struct Card *deck;
    char *statfilePath;
} Server;

/**
 * Type defination one entry of a statfile.
 */
typedef struct {
    char *port;
    int tokens;
    int points;
    int players;
} Stat;

/**
 * Type defination for properties of a statfile.
 */
typedef struct {
    int amount;
    Stat *stats;
} StatFileProp;

/**
 * Type defination arguments to the pthread when creating a
 * new game property on the server.
 */
typedef struct {
    Server *server;
    GameProp *prop;
} ServerGameArgs;

/**
 * Type defination arguments to the pthread when creating a
 * new game instance.
 */
typedef struct {
    GameProp *prop;
    int currentGameIndex;
} GameInstanceArgs;

#include "rafiki.h"

// Global variable for signal handling,
// freeing memory when sigint or sigterm is caught
Server *sigServer;

/**
 * Function prototypes.
 */
void free_server(Server *server);
void exit_with_error(int error);
void check_args(int argc, char **argv);
void load_deckfile(Server *server, char *path);
int check_stat_line(char *line);
Stat generate_stat(char *line);
int index_of_non_zero_port(StatFileProp prop, char *port);
StatFileProp load_statfile(char *path);
enum Error get_socket(int *output, char *port);
void add_score_entry(GameProp *prop, ScoreEntry entry);
enum ErrorCode update_scores(GameProp *prop, struct Game *game,
enum MessageFromPlayer type, char *message, int playerId);
enum ErrorCode do_what(GameProp *prop, struct Game *game, int playerId);
void send_all(struct Game *game, char *message, ...);
void *game_instance_thread(void *arg);
void setup_player_fd(struct GamePlayer *player, int sock);
void add_player(struct Game *game, struct GamePlayer *player,
        pthread_mutex_t *lock);
struct Game setup_instance(char *name, int token, int winScore);
void add_instance(GameProp *prop, struct Game game, pthread_mutex_t *lock);
int index_of_instance(GameProp *prop, char *name);
int setup_player(struct GamePlayer *player, int id);
int get_game_amount(Server *server, char *name);
int compare_name(const void *a, const void *b);
void assign_id(struct Game *game);
void send_game_initial_messages(Server *server, GameProp *prop,
        struct Game game);
void setup_scores_table(GameProp *prop, struct Game *instance);
void play_game(Server *server, GameProp *prop, int index,
        pthread_mutex_t *lock);
void create_new_game(GameProp *prop, struct GamePlayer *player,
        char *name, pthread_mutex_t *lock);
void add_to_existing_game(GameProp *prop, struct GamePlayer *player,
        int index, pthread_mutex_t *lock);
int get_avaliable_game(GameProp *prop, char *name);
int get_avaliable_game_all(Server *server, char *name, char **portOut);
GameProp *get_prop_by_port(Server *server, char *port);
int index_of_player_in_table(ScoreTable table, char *playerName);
void combine_all_scores_and_send(Server *server, FILE *toConnection);
enum ConnectionType verify_connection(GameProp *prop, FILE *toConnection,
        FILE *fromConnection);
void handle_player_connect(Server *server, GameProp *prop, FILE *toConnection,
        FILE *fromConnection, int sock, pthread_mutex_t gamePropLock);
void handle_connection(Server *server, GameProp *prop, int sock);
void *listen_thread(void *argv);
void start_server(Server *server);
void setup_server(Server *server);
void setup_game_sockets(Server *server, StatFileProp prop, char *key,
        int timeout);
void signal_handler(int sig);
void setup_signal_handler();

#endif