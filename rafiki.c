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
    char *port;
    char *key;
    pthread_t mainThread;
    int playerMax;
    int instanceSize;
    struct Game *instances;
    pthread_t *instanceThreads;
    int tokens;
    int points;
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

// Global variable for signal handling,
// freeing memory when sigint or sigterm is caught
Server *sigServer;

// Mutex locking when modifications to GameProp occurs.
pthread_mutex_t gamePropMutex = PTHREAD_MUTEX_INITIALIZER;

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
            if (instance.deckSize > 0) {
                free(instance.deck);
            }
            if (instance.playerCount > 0) {
                free(instance.players);
            }
            // pthread_join(prop.instanceThreads[j], NULL);
        }
        free(prop.instances);
        free(prop.instanceThreads);
    }
    free(server->gameProps);
    if (server->deckSize > 0) {
        free(server->deck);
    }
}

void exit_with_error(int error) {
    switch(error) {
        case INVALID_ARG_NUM:
            fprintf(stderr, "Usage: rafiki keyfile deckfile statfile"\
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

void load_statfile(char *path) {

}

int get_socket(char *port) {
    struct addrinfo hints, *res, *res0;
    int sock;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int error = getaddrinfo(LOCALHOST, port, &hints, &res0);
    if (error) {
        exit_with_error(FAILED_LISTEN);
    }
    if (strcmp(port, "0") == 0) {
        sock = -1;
    } else {
        sock = -1;
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
        exit_with_error(FAILED_LISTEN);
    }
    freeaddrinfo(res0);
    return sock;
}

void display_deck(struct Card *cards, int size) {
    for (int i = 0; i < size; i++) {
        printf("card %i %i\n", cards[i].points, cards[i].cost[0]);
    }
}

void *game_instance_thread(void *arg) {
    struct Game *game = (struct Game *) arg;
    printf("Game started!\n");
    return NULL;
}

void setup_player_fd(struct GamePlayer *player, int sock) {
    player->fileDescriptor = sock;
    player->toPlayer = fdopen(sock, "w");
    player->fromPlayer = fdopen(sock, "r");
}

void add_player(struct Game *game, struct GamePlayer *player) {
    printf("ADDING PLAYER %s TO GAME %s\n", player->state.name, game->name);
    int size = game->playerCount;
    pthread_mutex_lock(&gamePropMutex);
    game->players = realloc(game->players, sizeof(struct GamePlayer)
            * (size + 1));
    game->players[size] = *player;
    game->playerCount++;
    pthread_mutex_unlock(&gamePropMutex);
}

struct Game setup_instance(char *name) {
    struct Game instance;
    instance.players = malloc(0);
    instance.playerCount = 0;
    instance.name = malloc(sizeof(char) * strlen(name) + 1);
    strcpy(instance.name, name);
    free(name);
    instance.name[strlen(instance.name)] = '\0';
    instance.deckSize = 0;
    return instance;
}

void add_instance(GameProp *prop, struct Game game) {
    int size = prop->instanceSize;
    pthread_mutex_lock(&gamePropMutex);
    prop->instances = realloc(prop->instances, sizeof(struct Game) *
            (size + 1));
    prop->instances[size] = game;
    prop->instanceSize++;
    pthread_mutex_unlock(&gamePropMutex);
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
        // free_connection(connection);
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
                prop->tokens);
    }
}

void play_game(Server *server, GameProp *prop, int index) {
    struct Game *instance = &prop->instances[index];
    int size = prop->instanceSize;
    assign_id(instance);
    send_game_initial_messages(server, prop, *instance);
    display_deck(server->deck, server->deckSize);
    // Copy deck details in to game instance.
    instance->deckSize = server->deckSize;
    instance->deck = malloc(sizeof(struct Card) * server->deckSize);
    memcpy(instance->deck, server->deck, sizeof(struct Card) * server->deckSize);
    pthread_mutex_lock(&gamePropMutex);
    prop->instanceThreads = realloc(prop->instanceThreads, sizeof(pthread_t) *
        (size));
    pthread_create(&prop->instanceThreads[size - 1], NULL,
            game_instance_thread, (void *) instance);
        printf("thread %i\n", (int)prop->instanceThreads[size - 1]);
    pthread_mutex_unlock(&gamePropMutex);
}

void create_new_game(GameProp *prop, struct GamePlayer *player,
        char *name) {
    printf("SETTING UP NEW GAME\n");
    struct Game instance = setup_instance(name);
    setup_player(player, instance.playerCount);
    add_player(&instance, player);
    add_instance(prop, instance);
}

void add_to_existing_game(GameProp *prop, struct GamePlayer *player,
        int index) {
    setup_player(player, prop->instances[index].playerCount);
    add_player(&prop->instances[index], player);
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
        create_new_game(prop, &player, buffer);
        index = prop->instanceSize - 1;
    } else {
        free(buffer);
        if (strcmp(prop->port, port) != 0) {
            add_to_existing_game(get_prop_by_port(server, port), &player,
                    index);
            diffPort = 1;
        } else {
            add_to_existing_game(prop, &player, index);
        }
    }
    if (!diffPort) { // Game not on a different port.
        if (prop->playerMax == prop->instances[index].playerCount) {
            play_game(server, prop, index);
        }
    } else { // Game on a different port? Get proprties of the other port.
        if (prop->playerMax == get_prop_by_port(server, port)
                ->instances[index].playerCount) {
            play_game(server, get_prop_by_port(server, port), index);
        }
    }
}

typedef struct {
    Server *server;
    GameProp *prop;
} Args;

void *listen_thread(void *argv) {
    Args *args = (Args *) argv;
    Server *server = args->server;
    GameProp *prop = args->prop;
    printf("%s\n", prop->port);
    printf("LISTENER THREAD %i STARTED\n", (int) pthread_self());
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    int x = 0;
    while(1) {
        int sock = accept(prop->socket, (struct sockaddr *) &in, &size);
        if (sock == -1) {
            exit_with_error(FAILED_LISTEN);
        }
        handle_connection(server, prop, sock);
        if (x++ > 4) break;
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

    // pthread_detach(pthread_self());
    return NULL;
}

void start_server(Server *server) {
    Args *argList = malloc(sizeof(Args) * server->portAmount);
    for (int i = 0; i < server->portAmount; i++) {
        Args args;
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

void setup_ports(Server *server, char **ports) {
    server->ports = ports;
}

void setup_game_sockets(Server *server, char **ports, int amount) {
    server->gameProps = malloc(sizeof(GameProp) * amount);
    server->portAmount = amount;
    for (int i = 0; i < amount; i++) {
        server->gameProps[i].port = ports[i];
        server->gameProps[i].socket = get_socket(ports[i]);
        server->gameProps[i].key = "12345";
        server->gameProps[i].instanceSize = 0;
        server->gameProps[i].instances = malloc(sizeof(struct Game));
        server->gameProps[i].instanceThreads = malloc(0);
        server->gameProps[i].playerMax = 2;
        server->gameProps[i].tokens = 3;
        server->gameProps[i].points = 10;
    }
}

int main(int argc, char **argv) {
    setup_signal_handler();
    check_args(argc, argv);
    char *ports[2] = {"3000", "3001"};
    Server server;
    sigServer = &server;
    setup_server(&server);
    load_keyfile(argv[1]);
    load_deckfile(&server, argv[2]);
    load_statfile(argv[3]);
    setup_game_sockets(&server, ports, 2);
    start_server(&server);
    free_server(&server);
}