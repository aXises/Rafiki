#include "shared.h"

#define EXPECTED_STATFILE_SEP 3
#define EXPECTED_ARGC 5

enum Args {
    KEYFILE = 1,
    DECKFILE = 2,
    STATFILE = 3,
    TIMEOUT = 4
};

enum StatFile {
    PORT = 0,
    START_TOKENS = 1,
    START_POINTS = 2,
    START_PLAYERS = 3,
};

typedef struct {
    char *playerName;
    int tokensTaken;
    int pointsEarned;
} ScoreEntry;

typedef struct {
    int entryCount;
    ScoreEntry *entries;
} ScoreTable;

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

typedef struct {
    int timeout;
    int socket;
    int portAmount;
    GameProp *gameProps;
    char *key;
    char **ports;
    int deckSize;
    struct Card *deck;
} Server;

typedef struct {
   char *port;
   int tokens;
   int points;
   int players;
} Stat;

typedef struct {
    int amount;
    Stat *stats;
} StatFileProp;

typedef struct {
    Server *server;
    GameProp *prop;
} ServerGameArgs;

typedef struct {
    GameProp *prop;
    int currentGameIndex;
} GameInstanceArgs;

// Global variable for signal handling,
// freeing memory when sigint or sigterm is caught
Server *sigServer;

void free_server(Server *server) {
    for (int i = 0; i < server->portAmount; i++) {
        GameProp prop = server->gameProps[i];
        for (int j = 0; j < prop.instanceSize; j++) {
            struct Game instance = prop.instances[j];
            pthread_join(prop.instanceThreads[j], NULL);
            for (int k = 0; k < instance.playerCount; k++) {
                struct GamePlayer player = instance.players[k];
                fclose(player.toPlayer);
                fclose(player.fromPlayer);
                free(player.state.name);
            }
            free(instance.name);
            free(instance.deck);
            if (instance.playerCount > 0) {
                free(instance.players);
            }
        }
        free(prop.instances);
        free(prop.instanceThreads);
        free(prop.port);
        free(prop.key);
    }
    if (server->portAmount > 0) {
        free(server->gameProps);
    }
    if (server->deckSize > 0) {
        free(server->deck);
    }
}

void exit_with_error(int error) {
    switch(error) {
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: rafiki keyfile deckfile statfile "\
                    "timeout\n");
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
    if (argc != EXPECTED_ARGC || !is_string_digit(argv[TIMEOUT])) {
        exit_with_error(INVALID_ARG_NUM);
    }
}

void load_deckfile(Server *server, char *path) {
    enum DeckStatus status;
    status = parse_deck_file(&server->deckSize, &server->deck, path);
    switch(status) {
        case (VALID):
            break;
        case (DECK_ACCESS):
            exit_with_error(SYSTEM_ERR);
        case (DECK_INVALID):
            exit_with_error(INVALID_DECKFILE);
    };
}

int check_stat_line(char *line) {
    char *newLine = malloc(strlen(line) + 1);
    strcpy(newLine, line);
    newLine[strlen(line)] = '\0';
    if (!match_seperators(newLine, 0, EXPECTED_STATFILE_SEP)) {
        free(newLine);
        return 0;
    }
    int isValid = 1;
    char **commaSplit = split(newLine, ",");
    for (int i = 0; i < EXPECTED_STATFILE_SEP + 1; i++) {
        if (!is_string_digit(commaSplit[i]) || strcmp(commaSplit[i], "") == 0) {
            isValid = 0;
            break;
        }
    }
    free(commaSplit);
    free(newLine);
    return isValid;
}

Stat generate_stat(char *line) {
    Stat stat;
    char **contentSplit = split(line, ",");
    stat.port = malloc(strlen(contentSplit[PORT]) + 1);
    strcpy(stat.port, contentSplit[PORT]);
    stat.port[strlen(contentSplit[PORT])] = '\0';
    stat.tokens = atoi(contentSplit[START_TOKENS]);
    stat.points = atoi(contentSplit[START_POINTS]);
    stat.players = atoi(contentSplit[START_PLAYERS]);
    free(contentSplit);
    return stat;
}

int index_of_non_zero_port(StatFileProp prop, char *port) {
    int index = -1;
    if (strcmp(port, "0") == 0) {
        return -1;
    }
    for (int i = 0; i < prop.amount; i++) {
        if (strcmp(prop.stats[i].port, port) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

StatFileProp load_statfile(char *path) {
    StatFileProp prop;
    FILE *file = fopen(path, "r");
    if (file == NULL || !file) {
        exit_with_error(INVALID_STATFILE);
    }
    char *content = malloc(sizeof(char)), character;
    int counter = 0, lines = 0, isValid = 1;
    while((character = getc(file)) != EOF) {
        content = realloc(content, sizeof(char) * (counter + 1));
        content[counter] = character;
        if (character == '\n') {
            lines++;
        }
        counter++;
    }
    content = realloc(content, sizeof(char) * (counter + 1));
    content[counter] = '\0';
    if (lines == 0 && check_stat_line(content)) {
        prop.stats = malloc(sizeof(Stat));
        prop.stats[0] = generate_stat(content);
        prop.amount = 1;
    } else if (lines > 0) {
        char **splitContent = split(content, "\n");
        prop.stats = malloc(sizeof(Stat) * (lines + 1));
        prop.amount = 0;
        for (int i = 0; i < lines + 1; i++) {
            if (check_stat_line(splitContent[i])) {
                Stat stat = generate_stat(splitContent[i]);
                if (index_of_non_zero_port(prop, stat.port) != -1) {
                    isValid = 0;
                    break;
                }
                prop.stats[i] = stat;
                prop.amount++;
            } else {
                isValid = 0;
                break;
            }
        }
        free(splitContent);
    } else {
        isValid = 0;
    }
    fclose(file);
    free(content);
    if (!isValid) {
        free(prop.stats);
        exit_with_error(INVALID_STATFILE);
    }
    return prop;
}


enum Error get_socket(int *output, char *port) {
    struct addrinfo hints, *res, *res0;
    int sock;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = getaddrinfo(LOCALHOST, port, &hints, &res0);
    if (error) {
        freeaddrinfo(res0);
        return FAILED_LISTEN;
    }
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
        freeaddrinfo(res0);
        return FAILED_LISTEN;
    }
    freeaddrinfo(res0);
    *output = sock;
    return NOTHING_WRONG;
}

int scores_index_of_player(GameProp *prop, char *name) {
    int index = -1;
    for (int i = 0; i < prop->scoresTable.entryCount; i++) {
        if (strcmp(prop->scoresTable.entries[i].playerName, name) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

void add_score_entry(GameProp *prop, ScoreEntry entry) {
    int index = scores_index_of_player(prop, entry.playerName);
    ScoreTable *table = &prop->scoresTable;
    if (index == -1) {
        table->entries = realloc(table->entries, sizeof(ScoreEntry) *
                (table->entryCount + 1));
        table->entries[table->entryCount] = entry;
        table->entryCount++;
    } else {
        table->entries[index].tokensTaken += entry.tokensTaken;
        table->entries[index].pointsEarned += entry.pointsEarned;
    }
}

enum ErrorCode update_scores(GameProp *prop, struct Game *game,
        enum MessageFromPlayer type, char *message, int playerId) {
    ScoreEntry entry;
    entry.playerName = game->players[playerId].state.name;
    entry.tokensTaken = 0;
    entry.pointsEarned = 0;
    enum ErrorCode err = 0;
    if (type == PURCHASE) {
        struct PurchaseMessage purchaseMessage;
        err = parse_purchase_message(&purchaseMessage, message);
        if (err) {
            return err;
        }
        entry.pointsEarned = game->board[purchaseMessage.cardNumber].points;
    } else if (type == TAKE) {
        struct TakeMessage takeMessage;
        err = parse_take_message(&takeMessage, message);
        if (err) {
            return err;
        }
        entry.tokensTaken = takeMessage.tokens[TOKEN_PURPLE] +
                takeMessage.tokens[TOKEN_BROWN] +
                takeMessage.tokens[TOKEN_YELLOW] +
                takeMessage.tokens[TOKEN_RED];
    } else if (type == WILD) {
        entry.tokensTaken = 1;
    } else {
        return PROTOCOL_ERROR;
    }
    add_score_entry(prop, entry);
    return err;
}

/* Process one player's turn, from sending the do what message to being ready
 * to send the do what message to the next player. Does not handle retries in
 * the case where the player sends an invalid message.
 */
enum ErrorCode do_what(GameProp *prop, struct Game *game, int playerId) {
    enum ErrorCode err = 0;
    FILE* toPlayer = game->players[playerId].toPlayer;
    FILE* fromPlayer = game->players[playerId].fromPlayer;

    fputs("dowhat\n", toPlayer);
    fflush(toPlayer);

    char* line;
    int readBytes = read_line(fromPlayer, &line, 0);
    if (readBytes <= 0) {
        if (ferror(fromPlayer) && errno == EINTR) {
            free(line);
            return INTERRUPTED;
        } else if (feof(fromPlayer)) {
            free(line);
            return PLAYER_CLOSED;
        }
    }
    enum MessageFromPlayer type = classify_from_player(line);
    printf("parsing %s\n", line);
    if (update_scores(prop, game, type, line, playerId) != NOTHING_WRONG) {
        printf("fail here\n");
        free(line);
        return PROTOCOL_ERROR;
    };
    switch(type) {
        case PURCHASE:
            err = handle_purchase_message(playerId, game, line);
            break;
        case TAKE:
            err = handle_take_message(playerId, game, line);
            break;
        case WILD:
            handle_wild_message(playerId, game);
            break;
        default:
            free(line);
            return PROTOCOL_ERROR;
    }
    free(line);
    return err;
}

/**
* Send an message to all the players.
* @param game - The game instance.
* @param message - The message to send.
*/
void send_all(struct Game *game, char *message, ...) {
    for (int i = 0; i < game->playerCount; i++) {
        va_list args;
        va_start(args, message);
        vfprintf(game->players[i].toPlayer, message, args);
        fflush(game->players[i].toPlayer);
        va_end(args);
    }
}

void *game_instance_thread(void *arg) {
    // Start playing game
    GameProp *prop = (GameProp *) arg;
    struct Game *game = &prop->instances[prop->currentGameIndex];
    printf("Game started!\n");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        draw_card(game);
    }
    enum ErrorCode err = 0;
    while(!is_game_over(game)) {
        for (int i = 0; i < game->playerCount; i++) {
            err = do_what(prop, game, i);
                for (int i = 0; i < prop->scoresTable.entryCount; i++) {
                    ScoreEntry s = prop->scoresTable.entries[i];
                    printf("%s %i %i\n", s.playerName, s.tokensTaken, s.pointsEarned);
                }
            if (err == PROTOCOL_ERROR) {
                err = do_what(prop, game, i);
            }
            if (err == PLAYER_CLOSED) {
                char *message = print_disco_message(i + 'A');
                send_all(game, message);
                free(message);
                return NULL;
            }
            if (err) {
                char *message = print_invalid_message(i + 'A');
                send_all(game, message);
                free(message);
                return NULL;
            }
        }
    }
    send_all(game, "eog\n");
    return NULL;
}

void setup_player_fd(struct GamePlayer *player, int sock) {
    player->fileDescriptor = sock;
    player->toPlayer = fdopen(sock, "w");
    player->fromPlayer = fdopen(sock, "r");
}

void add_player(struct Game *game, struct GamePlayer *player,
        pthread_mutex_t *mutex) {
    printf("ADDING PLAYER %s TO GAME %s\n", player->state.name, game->name);
    int size = game->playerCount;
    pthread_mutex_lock(mutex);
    game->players = realloc(game->players, sizeof(struct GamePlayer)
            * (size + 1));
    game->players[size] = *player;
    game->playerCount++;
    pthread_mutex_unlock(mutex);
}

struct Game setup_instance(char *name, int token, int winScore) {
    struct Game instance;
    instance.players = malloc(0);
    instance.playerCount = 0;
    instance.name = malloc(sizeof(char) * strlen(name) + 1);
    strcpy(instance.name, name);
    free(name);
    instance.name[strlen(instance.name)] = '\0';
    instance.deckSize = 0;
    instance.boardSize = 0;
    instance.winScore = winScore;
    for (int i = 0; i < TOKEN_MAX - 1; i++) {
        instance.tokenCount[i] = token;
    }
    return instance;
}

void add_instance(GameProp *prop, struct Game game, pthread_mutex_t *mutex) {
    int size = prop->instanceSize;
    pthread_mutex_lock(mutex);
    prop->instances = realloc(prop->instances, sizeof(struct Game) *
            (size + 1));
    prop->instances[size] = game;
    prop->instanceSize++;
    pthread_mutex_unlock(mutex);
}

int index_of_instance(GameProp *prop, char *name) {
    int index = -1;
    for (int i = 0; i < prop->instanceSize; i++) {
        if (strcmp(prop->instances[i].name, name) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

int verify_player(char *key, struct GamePlayer *player) {
    char *buffer;
    read_line(player->fromPlayer, &buffer, 0);
    if (!strcmp(key, buffer) == 0) {
        send_message(player->toPlayer, "no\n");
        return 0;
    }
    send_message(player->toPlayer, "yes\n");
    free(buffer);
    return 1;
}

void setup_player(struct GamePlayer *player, int id) {
    struct Player state;
    initialize_player(&state, id);
    read_line(player->fromPlayer, &state.name, 0);
    player->state = state;
}

int get_game_amount(Server *server, char *name) {
    int counter = 0;
    for (int i = 0; i < server->portAmount; i++) {
        GameProp prop = server->gameProps[i];
        for (int j = 0; j < prop.instanceSize; j++) {
            if (strcmp(prop.instances[j].name, name) == 0) {
                counter++;
            }
        }
    }
    return counter;
}

int compare_name(const void *a, const void *b) {
    struct GamePlayer *playerA = (struct GamePlayer *) a;
    struct GamePlayer *playerB = (struct GamePlayer *) b;
    char *nameA = playerA->state.name;
    char *nameB = playerB->state.name;
    return strcmp(nameA, nameB);
}

void assign_id(struct Game *game) {
    qsort(game->players, game->playerCount, sizeof(struct GamePlayer),
            compare_name);
    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].state.playerId = i;
    }
}

void send_game_initial_messages(Server *server, GameProp *prop,
        struct Game game) {
    int gameCounter = get_game_amount(server, game.name);
    for (int i = 0; i < game.playerCount; i++) {
        send_message(game.players[i].toPlayer, "rid%s,%i,%i\n", game.name,
                gameCounter, game.players[i].state.playerId);
        send_message(game.players[i].toPlayer, "playinfo%i/%i\n",
                game.players[i].state.playerId, game.playerCount);
        send_message(game.players[i].toPlayer, "tokens%i\n",
                prop->startToken);
    }
}

void setup_scores_table(GameProp *prop, struct Game *instance) {
    prop->scoresTable.entryCount = 0;
    prop->scoresTable.entries = malloc(0);
    for (int i = 0; i < instance->playerCount; i++) {
        ScoreEntry entry;
        entry.playerName = instance->players[i].state.name;
        entry.tokensTaken = 0;
        entry.pointsEarned = 0;
        add_score_entry(prop, entry);
    }
}

void play_game(Server *server, GameProp *prop, int index,
        pthread_mutex_t *mutex) {
    struct Game *instance = &prop->instances[index];
    int size = prop->instanceSize;
    assign_id(instance);
    setup_scores_table(prop, instance);
    send_game_initial_messages(server, prop, *instance);
    // Copy deck details in to game instance.
    instance->deckSize = server->deckSize;
    instance->deck = malloc(sizeof(struct Card) * server->deckSize);
    memcpy(instance->deck, server->deck, sizeof(struct Card) *
            server->deckSize);
    pthread_mutex_lock(mutex);
    prop->instanceThreads = realloc(prop->instanceThreads, sizeof(pthread_t) *
            (size));
    prop->currentGameIndex = index;
    pthread_create(&prop->instanceThreads[size - 1], NULL,
            game_instance_thread, (void *) prop);
    pthread_mutex_unlock(mutex);
}

void create_new_game(GameProp *prop, struct GamePlayer *player,
        char *name, pthread_mutex_t *mutex) {
    printf("SETTING UP NEW GAME\n");
    struct Game instance = setup_instance(name, prop->startToken,
            prop->winPoints);
    setup_player(player, instance.playerCount);
    add_player(&instance, player, mutex);
    add_instance(prop, instance, mutex);
}

void add_to_existing_game(GameProp *prop, struct GamePlayer *player,
        int index, pthread_mutex_t *mutex) {
    setup_player(player, prop->instances[index].playerCount);
    add_player(&prop->instances[index], player, mutex);
}

int get_avaliable_game(GameProp *prop, char *name) {
    int index = -1;
    for (int i = 0; i < prop->instanceSize; i++) {
        if (strcmp(prop->instances[i].name, name) == 0 &&
                !(prop->instances[i].playerCount >= prop->playerMax)) {
            index = i;
            break;
        }
    }
    return index;
}

int get_avaliable_game_all(Server *server, char *name, char **portOut) {
    int index = -1;
    for (int i = 0; i < server->portAmount; i++) {
        index = get_avaliable_game(&server->gameProps[i], name);
        if (index != -1) {
            *portOut = server->gameProps[i].port;
            break;
        }
    }
    return index;
}

GameProp *get_prop_by_port(Server *server, char *port) {
    for (int i = 0; i < server->portAmount; i++) {
        if (strcmp(server->gameProps[i].port, port) == 0) {
            return &server->gameProps[i];
        }
    }
    return NULL;
}

void handle_connection(Server *server, GameProp *prop, int sock) {
    pthread_mutex_t gamePropLock = PTHREAD_MUTEX_INITIALIZER;
    struct GamePlayer player;
    setup_player_fd(&player, sock);
    if (!verify_player(prop->key, &player)) {
        // free
        return;
    }
    char *buffer;
    read_line(player.fromPlayer, &buffer, 0);
    if (strcmp(buffer, "reconnect") == 0) {
        // Handle reconnect
        free(buffer);
        return;
    }
    char *port;
    int diffPort = 0;
    int index = get_avaliable_game_all(server, buffer, &port);
    if (index == -1) {
        create_new_game(prop, &player, buffer, &gamePropLock);
        index = prop->instanceSize - 1;
    } else {
        free(buffer);
        if (strcmp(prop->port, port) != 0) {
            add_to_existing_game(get_prop_by_port(server, port), &player,
                    index, &gamePropLock);
            diffPort = 1;
        } else {
            add_to_existing_game(prop, &player, index, &gamePropLock);
        }
    }
    if (!diffPort) { // Game not on a different port.
        if (prop->playerMax == prop->instances[index].playerCount) {
            play_game(server, prop, index, &gamePropLock);
        }
    } else { // Game on a different port? Get proprties of the other port.
        if (prop->playerMax == get_prop_by_port(server, port)
                ->instances[index].playerCount) {
            play_game(server, get_prop_by_port(server, port), index,
                    &gamePropLock);
        }
    }
}

void *listen_thread(void *argv) {
    ServerGameArgs *args = (ServerGameArgs *) argv;
    Server *server = args->server;
    GameProp *prop = args->prop;
    printf("%s %i\n", prop->port, prop->socket);
    printf("LISTENER THREAD %i STARTED\n", (int) pthread_self());
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    while(1) {
        int sock = accept(prop->socket, (struct sockaddr *) &in, &size);
        if (sock == -1) {
            printf("fail here\n");
            exit_with_error(FAILED_LISTEN);
        }
        handle_connection(server, prop, sock);
    }
    for (int i = 0; i < prop->instanceSize; i++) {
        pthread_join(prop->instanceThreads[i], NULL);
        struct Game game = prop->instances[i];
        printf("Game instance %i has %i players\n", i, game.playerCount);
        for (int j = 0; j < game.playerCount; j++) {
            struct GamePlayer p = game.players[j];
            printf("- %s : %i\n", p.state.name, p.state.playerId);
        }
    }
    return NULL;
}

void start_server(Server *server) {
    ServerGameArgs *argList = malloc(sizeof(ServerGameArgs) *
            server->portAmount);
    for (int i = 0; i < server->portAmount; i++) {
        ServerGameArgs args;
        args.server = server;
        args.prop = &server->gameProps[i];
        argList[i] = args;
        pthread_create(&server->gameProps[i].mainThread, NULL, listen_thread,
                (void *) &argList[i]);
    }
    for (int i = 0; i < server->portAmount; i++) {
        pthread_join(server->gameProps[i].mainThread, NULL);
    }
    free(argList);
}

void setup_server(Server *server) {
    server->timeout = 0;
    server->portAmount = 0;
    server->key = "12345";
    server->deckSize = 0;
}

void signal_handler(int sig) {
    switch(sig) {
        case(SIGINT):
            printf("SIGINT CAUGHT\n");
            for (int i = 0; i < sigServer->portAmount; i++) {
                GameProp prop = sigServer->gameProps[i];
                for (int j = 0; j < prop.instanceSize; j++) {
                    pthread_cancel(prop.instanceThreads[j]);
                    pthread_join(prop.instanceThreads[j], NULL);
                }
                pthread_cancel(prop.mainThread);
                pthread_join(prop.mainThread, NULL);
            }
            free_server(sigServer);
            exit(0);
            break;
        case(SIGTERM):
            printf("SIGTERM CAUGHT\n");
            exit(0);
            break;
    }
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void setup_game_sockets(Server *server, StatFileProp prop, char *key,
        int timeout) {
    server->gameProps = malloc(sizeof(GameProp) * prop.amount);
    server->portAmount = prop.amount;
    for (int i = 0; i < prop.amount; i++) {
        enum Error err = get_socket(&server->gameProps[i].socket, prop.stats[i].port);
        if (err) {
            exit_with_error(err);
        }
        server->gameProps[i].port = malloc(sizeof(char) *
                (strlen(prop.stats[i].port) + 1));
        strcpy(server->gameProps[i].port, prop.stats[i].port);
        server->gameProps[i].port[strlen(prop.stats[i].port)] = '\0';
        server->gameProps[i].key = malloc(sizeof(char) * (strlen(key) + 1));
        strcpy(server->gameProps[i].key, key);
        server->gameProps[i].key[strlen(key)] = '\0';
        server->gameProps[i].instanceSize = 0;
        server->gameProps[i].instances = malloc(sizeof(struct Game));
        server->gameProps[i].instanceThreads = malloc(0);
        server->gameProps[i].playerMax = prop.stats[i].players;
        server->gameProps[i].startToken = prop.stats[i].tokens;
        server->gameProps[i].winPoints = prop.stats[i].points;
        server->gameProps[i].timeout = timeout;
        server->gameProps[i].currentGameIndex = -1;
    }
    for (int i = 0; i < prop.amount; i++) {
        free(prop.stats[i].port);
    }
    free(prop.stats);
    free(key);
}

int main(int argc, char **argv) {
    setup_signal_handler();
    check_args(argc, argv);
    Server server;
    sigServer = &server;
    setup_server(&server);
    char *key;
    enum Error err = load_keyfile(&key, argv[KEYFILE]);
    if (err) {
        exit_with_error(err);
    }
    load_deckfile(&server, argv[DECKFILE]);
    StatFileProp prop = load_statfile(argv[STATFILE]);
    setup_game_sockets(&server, prop, key, atoi(argv[TIMEOUT]));
    start_server(&server);
    free_server(&server);
}