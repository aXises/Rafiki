#include "rafiki.h"

// Global variable for signal handling,
// freeing memory when sigint or sigterm is caught
Server *sigServer;

/**
 * Frees memory allocated to the main server.
 * @param server - The server instance.
 */
void free_server(Server *server) {
    for (int i = 0; i < server->portAmount; i++) {
        GameProp prop = server->gameProps[i];
        for (int j = 0; j < prop.instanceSize; j++) {
            struct Game instance = prop.instances[j];
            for (int k = 0; k < instance.playerCount; k++) {
                struct GamePlayer player = instance.players[k];
                fclose(player.toPlayer);
                fclose(player.fromPlayer);
                free(player.state.name);
            }
            free(instance.name);
            if (instance.playerCount == prop.playerMax) {
                free(instance.deck);
            }
            if (instance.playerCount > 0) {
                free(instance.players);
            }
        }
        free(prop.instances);
        free(prop.instanceThreads);
        free(prop.port);
        free(prop.key);
        free(prop.scoresTable.entries);
    }
    if (server->portAmount > 0) {
        free(server->gameProps);
    }
    if (server->deckSize > 0) {
        free(server->deck);
    }
}

/**
 * Exits the program with a error code.
 * @param int - Error code.
 */
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

/**
 * Checks initial arguments for rafiki.
 * @param argc - Argument count.
 * @param argv - Argument vector.
 */
void check_args(int argc, char **argv) {
    if (argc != EXPECTED_ARGC) {
        exit_with_error(INVALID_ARG_NUM);
    }
    if (!is_string_digit(argv[TIMEOUT])) {
        exit_with_error(BAD_TIMEOUT);
    }
}

/**
 * Loads an deckfile to the server.
 * @param server - The server instance.
 * @param path - The path to the deckfile.
 */
void load_deckfile(Server *server, char *path) {
    enum DeckStatus status;
    status = parse_deck_file(&server->deckSize, &server->deck, path);
    switch(status) {
        case (VALID):
            break;
        case (DECK_ACCESS):
            exit_with_error(INVALID_DECKFILE);
        case (DECK_INVALID):
            exit_with_error(INVALID_DECKFILE);
    }
}

/**
 * Check and validates one line of the statfile
 * @param line - The string to check.
 * @returns 1 if the line is valid.
 */
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
        if (!is_string_digit(commaSplit[i]) ||
                strcmp(commaSplit[i], "") == 0) {
            isValid = 0;
            break;
        }
    }
    free(commaSplit);
    free(newLine);
    return isValid;
}

/**
 * Generates one stat property from a statfile line.
 * @param line - The line to generate from.
 * @returns one entry to the statfile property.
 */
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

/**
 * Gets the index of an non zero port.
 * @param prop - Statfile properties
 * @returns index of the port, -1 if the port is not in the property.
 */
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

/**
 * Loads a statfile.
 * @param path - The path to the statfile.
 * @returns a statfile type loaded with values from the statfile.
 */
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
        content[counter++] = character;
        if (character == '\n') {
            lines++;
        }
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
        for (int i = 0; i < lines; i++) {
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

/**
 * Generates a socket from a provided port.
 * @param output - The output socket.
 * @param port - Port value to generate the socket from.
 */
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
    int optVal = 1;
    if (setsockopt(*output, SOL_SOCKET, SO_REUSEADDR, &optVal,
            sizeof(int)) < 0) {
        return FAILED_LISTEN;
    }
    return NOTHING_WRONG;
}


/**
 * Adds one score entry type to the score table.
 * @param prop - The current game properties.
 * @param entry - The score entry to add.
 */
void add_score_entry(GameProp *prop, ScoreEntry entry) {
    int index = index_of_player_in_table(prop->scoresTable, entry.playerName);
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

/**
 * Update scores according to messages recieved from players.
 * @param prop - The current game properties.
 * @param game - The current game instance.
 * @param type - The message type received from the player.
 * @param message - The message received from the player.
 * @param playerId - The ID of the player.
 * @return a error code depending on whether if the message is valid.
 */
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
 * @param prop - The current game properties.
 * @param game - The current game instance.
 * @param playerId - The ID of the player.
 * @return a error code depending on whether if the message is valid.
 */
enum ErrorCode do_what(GameProp *prop, struct Game *game, int playerId) {
    enum ErrorCode err = 0;
    FILE *toPlayer = game->players[playerId].toPlayer;
    FILE *fromPlayer = game->players[playerId].fromPlayer;

    fputs("dowhat\n", toPlayer);
    fflush(toPlayer);

    char *line;
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
    if (update_scores(prop, game, type, line, playerId) != NOTHING_WRONG) {
        free(line);
        return PROTOCOL_ERROR;
    }
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

/**
 * A thread for handling one instance of a game.
 * @param arg - The GameInstanceArg type.
 */
void *game_instance_thread(void *arg) {
    pthread_detach(pthread_self());
    // Start playing game
    GameProp *prop = (GameProp *) arg;
    struct Game *game = &prop->instances[prop->currentGameIndex];
    //printf("Game started!\n");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        draw_card(game);
    }
    enum ErrorCode err = 0;
    while(!is_game_over(game)) {
        for (int i = 0; i < game->playerCount; i++) {
            err = do_what(prop, game, i);
            if (err == PROTOCOL_ERROR) {
                err = do_what(prop, game, i);
            }
            if (err == PLAYER_CLOSED) {
                char *message = print_disco_message(i);
                send_all(game, message);
                free(message);
                return NULL;
            }
            if (err) {
                char *message = print_invalid_message(i);
                send_all(game, message);
                free(message);
                return NULL;
            }
        }
    }
    send_all(game, "eog\n");
    return NULL;
}

/**
 * Sets up the player file descriptors based on a socket.
 * @param player - The player which will be setup.
 * @param sock - The socket to setup from.
 */
void setup_player_fd(struct GamePlayer *player, int sock) {
    player->fileDescriptor = sock;
    player->toPlayer = fdopen(sock, "w");
    player->fromPlayer = fdopen(sock, "r");
}

/**
 * Adds a player to a game instance.
 * @param player - The player to add.
 * @param sock - The game instance to add to.
 */
void add_player(struct Game *game, struct GamePlayer *player,
        pthread_mutex_t *lock) {
    //printf("ADDING PLAYER %s TO GAME %s\n", player->state.name, game->name);
    int size = game->playerCount;
    pthread_mutex_lock(lock);
    game->players = realloc(game->players, sizeof(struct GamePlayer)
            * (size + 1));
    game->players[size] = *player;
    game->playerCount++;
    pthread_mutex_unlock(lock);
}

/**
 * Sets up one instance of a game.
 * @param name - The name of the game.
 * @param token - The starting tokens in this game.
 * @param winScore - The required score to win.
 */
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

/**
 * Adds a game instance in to the game property it is derived from.
 * @param prop - The current game properties.
 * @param game - The current game instance.
 * @param lock - A mutex to prevent modifications to the game property.
 */
void add_instance(GameProp *prop, struct Game game, pthread_mutex_t *lock) {
    int size = prop->instanceSize;
    pthread_mutex_lock(lock);
    prop->instances = realloc(prop->instances, sizeof(struct Game) *
            (size + 1));
    prop->instances[size] = game;
    prop->instanceSize++;
    pthread_mutex_unlock(lock);
}

/**
 * Retrieves the index of a instance.
 * @param prop - The current game properties.
 * @param name - The name of the instance.
 * @returns index of the instance, -1 if the instance is not in the property.
 */
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

/**
 * Sets up a single player.
 * @param player - The player to setup.
 * @param id - The id to give to this player.
 * @returns 1 if the player was successfully setup, 0 otherwise.
 */
int setup_player(struct GamePlayer *player, int id) {
    struct Player state;
    initialize_player(&state, id);
    if (read_line(player->fromPlayer, &state.name, 0) <= 0) {
        return 0;
    }
    player->state = state;
    return 1;
}

/**
 * Gets the total amount of games with a particular name active.
 * @param server - The server instance
 * @param name - The name of the game.
 * @returns The amount of games running with the provided name.
 */
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

/**
 * Compares the names of two players. Used for qsort.
 * @param a - The first player.
 * @param b - The second player.
 */
int compare_name(const void *a, const void *b) {
    struct GamePlayer *playerA = (struct GamePlayer *) a;
    struct GamePlayer *playerB = (struct GamePlayer *) b;
    char *nameA = playerA->state.name;
    char *nameB = playerB->state.name;
    return strcmp(nameA, nameB);
}

/**
 * Assigns all players in a game an ID.
 * @param game - The current game instance.
 */
void assign_id(struct Game *game) {
    qsort(game->players, game->playerCount, sizeof(struct GamePlayer),
            compare_name);
    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].state.playerId = i;
    }
}

/**
 * Sends initial messages to all players to setup their game states.
 * @param server - The server instance.
 * @param prop - The properties of the game.
 * @param game - The game instance.
 */
void send_game_initial_messages(Server *server, GameProp *prop,
        struct Game game) {
    int gameCounter = get_game_amount(server, game.name);
    for (int i = 0; i < game.playerCount; i++) {
        send_message(game.players[i].toPlayer, "rid%s,%i,%i\n", game.name,
                gameCounter, game.players[i].state.playerId);
        send_message(game.players[i].toPlayer, "playinfo%c/%i\n",
                game.players[i].state.playerId + 'A', game.playerCount);
        send_message(game.players[i].toPlayer, "tokens%i\n",
                prop->startToken);
    }
}

/**
 * Sets up the scores table of a game property.
 * @param prop - The properties of the game.
 * @param instance - The game instance.
 */
void setup_scores_table(GameProp *prop, struct Game *instance) {
    for (int i = 0; i < instance->playerCount; i++) {
        ScoreEntry entry;
        entry.playerName = instance->players[i].state.name;
        entry.tokensTaken = 0;
        entry.pointsEarned = 0;
        add_score_entry(prop, entry);
    }
}

/**
 * Begins to play a game by sending the required messages and creating a
 * thread for the game.
 * @param server - The server instance.
 * @param prop - The properties of the game.
 * @param index - The index of the current game.
 * @param lock - Mutex for preventing game properties from being modified.
 */
void play_game(Server *server, GameProp *prop, int index,
        pthread_mutex_t *lock) {
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
    pthread_mutex_lock(lock);
    prop->instanceThreads = realloc(prop->instanceThreads, sizeof(pthread_t) *
            (size));
    prop->currentGameIndex = index;
    pthread_create(&prop->instanceThreads[size - 1], NULL,
            game_instance_thread, (void *) prop);
    pthread_mutex_unlock(lock);
}

/**
 * Creates a new game when a player attempts to join a game that does
 * not exist.
 * @param prop - The properties of the game.
 * @param index - The player that attempted to join this game.
 * @param name - The name of the game.
 * @param lock - Mutex for preventing game properties from being modified.
 */
void create_new_game(GameProp *prop, struct GamePlayer *player,
        char *name, pthread_mutex_t *lock) {
    //printf("SETTING UP NEW GAME\n");
    struct Game instance = setup_instance(name, prop->startToken,
            prop->winPoints);
    if (!setup_player(player, instance.playerCount)) {
        free(instance.players);
        free(instance.name);
        return;
    }
    add_player(&instance, player, lock);
    add_instance(prop, instance, lock);
}

/**
 * Adds a player to a existing game with the same name.
 * @param prop - The properties of the game.
 * @param index - The player that attempted to join this game.
 * @param index - The index of the game.
 * @param lock - Mutex for preventing game properties from being modified.
 */
void add_to_existing_game(GameProp *prop, struct GamePlayer *player,
        int index, pthread_mutex_t *lock) {
    setup_player(player, prop->instances[index].playerCount);
    add_player(&prop->instances[index], player, lock);
}

/**
 * Gets the index of a non full game with a particulat name on
 * a given game property.
 * @param index - The player that attempted to join this game.
 * @param name - The name of the game.
 * @returns The index of the game, -1 if the game does not exist.
 */
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

/**
 * Gets the index of a non full game with a particulat name on
 * all game properties (i.e all ports).
 * @param server - The server instance.
 * @param name - The name of the game.
 * @param portOut - The port the game exists on.
 * @returns The index of the game, -1 if the game does not exist.
 */
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

/**
 * Gets the properties of a game based on the port.
 * all game properties (i.e all ports).
 * @param server - The server instance.
 * @param port - The port to retrieve game properties from.
 * @returns The game properties associated with the port, NULL if none found.
 */
GameProp *get_prop_by_port(Server *server, char *port) {
    for (int i = 0; i < server->portAmount; i++) {
        if (strcmp(server->gameProps[i].port, port) == 0) {
            return &server->gameProps[i];
        }
    }
    return NULL;
}

/**
 * Gets the index of a player in a given score table based on the player name.
 * @param table - The score table to parse..
 * @param playerName - The name of the player.
 * @returns The index of the player, -1 if the player does not exist.
 */
int index_of_player_in_table(ScoreTable table, char *playerName) {
    int index = -1;
    for (int i = 0; i < table.entryCount; i++) {
        if (strcmp(table.entries[i].playerName, playerName) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

/**
 * Combines all scores on a server and sends it to a file descriptor.
 * @param server - The server instance.
 * @param toConnection - The connection to send to.
 */
void combine_all_scores_and_send(Server *server, FILE *toConnection) {
    ScoreTable table;
    table.entryCount = 0;
    table.entries = malloc(0);
    for (int i = 0; i < server->portAmount; i++) {
        for (int j = 0; j < server->gameProps[i].scoresTable.entryCount;
                j++) {
            ScoreEntry entry = server->gameProps[i].scoresTable.entries[j];
            int index = index_of_player_in_table(table, entry.playerName);
            if (index == -1) {
                table.entries = realloc(table.entries, sizeof(ScoreEntry) *
                        (table.entryCount + 1));
                table.entries[table.entryCount] = entry;
                table.entryCount++;
            } else {
                table.entries[index].tokensTaken += entry.tokensTaken;
                table.entries[index].pointsEarned += entry.pointsEarned;
            }
        }
    }
    send_message(toConnection, "Player Name,Total Tokens,Total Points\n");
    for (int i = 0; i < table.entryCount; i++) {
        ScoreEntry s = table.entries[i];
        send_message(toConnection, "%s,%i,%i\n", s.playerName,
                s.tokensTaken, s.pointsEarned);
    }
    free(table.entries);
}

/**
 * Verifies a connection to the server.
 * @param prop - The game properties.
 * @param toConnection - The connection to send to.
 * @param fromConnection - The connection to recieve from.
 * @returns The connection type.
 */
enum ConnectionType verify_connection(GameProp *prop, FILE *toConnection,
        FILE *fromConnection) {
    enum ConnectionType type;
    char *buffer;
    read_line(fromConnection, &buffer, 0);
    if (strcmp(buffer, "scores") == 0) {
        send_message(toConnection, "yes\n");
        type = SCORES_CONNECT;
    } else if (strstr(buffer, "play") != NULL) {
        char **encoded = split(buffer, "y");
        if (strcmp(prop->key, encoded[RIGHT]) != 0) {
            send_message(toConnection, "no\n");
            type = INVALID_CONNECT;
        } else {
            send_message(toConnection, "yes\n");
            type = PLAYER_CONNECT;
        }
        free(encoded);
    } else if (strstr(buffer, "reconnect") != NULL) {
        char **encoded = split(buffer, "t");
        if (strcmp(prop->key, encoded[RIGHT]) != 0) {
            send_message(toConnection, "no\n");
            type = INVALID_CONNECT;
        } else {
            send_message(toConnection, "yes\n");
            type = PLAYER_RECONNECT;
        }
        free(encoded);
    }
    free(buffer);
    return type;
}

/**
 * Handles a player reconnecting to the server.
 * @param server - The server instance.
 * @param prop - The game properties.
 * @param toConnection - The connection to send to.
 * @param fromConnection - The connection to recieve from.
 * @param sock - The port socket.
 * @param gamePropLock - Mutex to prevent game properties from being modified.
 */
void handle_player_reconnect(Server *server, GameProp *prop,
        FILE *toConnection, FILE *fromConnection, int sock,
        pthread_mutex_t gamePropLock) {
    struct GamePlayer player;
    player.fileDescriptor = sock;
    player.toPlayer = toConnection;
    player.fromPlayer = fromConnection;
    char *buffer;
    if (read_line(player.fromPlayer, &buffer, 0) <= 0) {
        fclose(player.toPlayer);
        fclose(player.fromPlayer);
        free(buffer);
        return;
    }
    // Add player to game.
    send_message(player.toPlayer, "no\n");
    fclose(player.toPlayer);
    fclose(player.fromPlayer);
    free(buffer);
}

/**
 * Handles a player connecting to the server.
 * @param server - The server instance.
 * @param prop - The game properties.
 * @param toConnection - The connection to send to.
 * @param fromConnection - The connection to recieve from.
 * @param sock - The port socket.
 * @param gamePropLock - Mutex to prevent game properties from being modified.
 */
void handle_player_connect(Server *server, GameProp *prop, FILE *toConnection,
        FILE *fromConnection, int sock, pthread_mutex_t gamePropLock) {
    struct GamePlayer player;
    player.fileDescriptor = sock;
    player.toPlayer = toConnection;
    player.fromPlayer = fromConnection;
    char *buffer;
    if (read_line(player.fromPlayer, &buffer, 0) <= 0) {
        fclose(player.toPlayer);
        fclose(player.fromPlayer);
        free(buffer);
        return;
    }
    char *port;
    int diffPort = 0;
    int index = get_avaliable_game_all(server, buffer, &port);
    if (index == -1) { // Game does not exist, create it.
        create_new_game(prop, &player, buffer, &gamePropLock);
        index = prop->instanceSize - 1;
    } else { // Game exists, add to existing game.
        free(buffer);
        if (strcmp(prop->port, port) != 0) {
            add_to_existing_game(get_prop_by_port(server, port), &player,
                    index, &gamePropLock);
            diffPort = 1;
        } else {
            add_to_existing_game(prop, &player, index, &gamePropLock);
        }
    }
    if (!diffPort) { // Game not on a different port, play the game.
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

/**
 * Handle a connection to the server
 * @param server - The server instance.
 * @param prop - The game properties.
 * @param sock - The socket to accept connections on.
 */
void handle_connection(Server *server, GameProp *prop, int sock) {
    pthread_mutex_t gamePropLock = PTHREAD_MUTEX_INITIALIZER;
    FILE *toConnection = fdopen(sock, "w");
    FILE *fromConnection = fdopen(sock, "r");
    enum ConnectionType type = verify_connection(prop, toConnection,
            fromConnection);
    switch(type) {
        case (PLAYER_CONNECT):
            handle_player_connect(server, prop, toConnection, fromConnection,
                    sock, gamePropLock);
            break;
        case (SCORES_CONNECT):
            combine_all_scores_and_send(server, toConnection);
            fclose(toConnection);
            fclose(fromConnection);
            break;
        case (PLAYER_RECONNECT):
            handle_player_reconnect(server, prop, toConnection,
                    fromConnection, sock, gamePropLock);
            break;
        case (INVALID_CONNECT):
            fclose(toConnection);
            fclose(fromConnection);
            break;
    }
}

/**
 * A thread for listening for connections on a particular port.
 * @param argv - ServerGameArgs type for passing multiple structs to a
 * thread.
 */
void *listen_thread(void *argv) {
    pthread_detach(pthread_self());
    ServerGameArgs *args = (ServerGameArgs *) argv;
    Server *server = args->server;
    GameProp *prop = args->prop;
    //printf("%s %i\n", prop->port, prop->socket);
    //printf("LISTENER THREAD %i STARTED\n", (int) pthread_self());
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    while(1) {
        int sock = accept(prop->socket, (struct sockaddr *) &in, &size);
        if (sock == -1) {
            exit_with_error(FAILED_LISTEN);
        }
        handle_connection(server, prop, sock);
    }
    return NULL;
}

/**
 * Starts the server by creating listener threads.
 * @param server - The server instance.
 */
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

/**
 * Sets up initial conditions of the server.
 */
void setup_server(Server *server) {
    server->portAmount = 0;
    server->deckSize = 0;
}

/**
 * Sets up sockets and properties associated with a port the server
 * is listening on.
 * @param server - The server instance.
 * @param prop - Properties of the statfile to use.
 * @param key - The auth key.
 * @param timeout - Connection timeout rate.
 */
void setup_game_sockets(Server *server, StatFileProp prop, char *key,
        int timeout) {
    server->gameProps = malloc(sizeof(GameProp) * prop.amount);
    server->portAmount = prop.amount;
    for (int i = 0; i < prop.amount; i++) {
        enum Error err = get_socket(&server->gameProps[i].socket,
                prop.stats[i].port);
        if (err) {
            exit_with_error(err);
        }
        struct sockaddr_in in;
        socklen_t len = sizeof(in);
        getsockname(server->gameProps[i].socket, (struct sockaddr *) &in,
                &len);
        char buffer[6];
        sprintf(buffer, "%i", ntohs(in.sin_port));
        server->gameProps[i].port = malloc(sizeof(char) *
                (strlen(buffer) + 1));
        strcpy(server->gameProps[i].port, buffer);
        server->gameProps[i].port[strlen(buffer)] = '\0';
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
        server->gameProps[i].scoresTable.entryCount = 0;
        server->gameProps[i].scoresTable.entries = malloc(0);
    }
    for (int i = 0; i < prop.amount; i++) {
        free(prop.stats[i].port);
    }
    free(prop.stats);
    free(key);
}

/**
 * Signal handler
 */
void signal_handler(int sig) {
    switch(sig) {
        case (SIGINT):
            printf("SIGINT CAUGHT\n");
            for (int i = 0; i < sigServer->portAmount; i++) {
                GameProp prop = sigServer->gameProps[i];
                if (close(prop.socket) == -1) {
                    exit_with_error(SYSTEM_ERR);
                }
            }
            free_server(sigServer);
            // StatFileProp prop = load_statfile(sigServer->statfilePath);
            // setup_game_sockets(sigServer, prop, sigServer->key,
            //         sigServer->timeout);
            // for (int i = 0; i < prop.amount; i++) {
            //     if (i == (prop.amount - 1)) {
            //         fprintf(stderr, "%s\n", sigServer->gameProps[i].port);
            //     } else {
            //         fprintf(stderr, "%s ", sigServer->gameProps[i].port);
            //     }
            // }
            // start_server(sigServer);
            exit(0);
            break;
        case (SIGTERM):
            // printf("SIGTERM CAUGHT\n");
            exit(0);
            break;
    }
}

/**
 * Sets up signal handler for SIGINT and SIGTERM.
 */
void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * Main
 */
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
    server.statfilePath = argv[STATFILE];
    StatFileProp prop = load_statfile(argv[STATFILE]);
    setup_game_sockets(&server, prop, key, atoi(argv[TIMEOUT]));
    for (int i = 0; i < prop.amount; i++) {
        if (i == (prop.amount - 1)) {
            fprintf(stderr, "%s\n", server.gameProps[i].port);
        } else {
            fprintf(stderr, "%s ", server.gameProps[i].port);
        }
    }
    start_server(&server);
    free_server(&server);
}