#include <server.h>
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

enum GameState {
    WAITING = 1,
    READY = 2,
    PLAYING = 3,
    EXITING = 4
};

typedef struct {
    int socket;
    char *port;
    char *key;
    pthread_t mainThread;
    pthread_t listenThread;
    int playerMax;
    int instanceSize;
    struct Game *instances;
    pthread_t *instanceThreads;
} GameProp;

typedef struct {
    int timeout;
    int socket;
    int gameAmount;
    GameProp *games;
    char *key;
    char **ports;
} Server;

// Global variable for signal handling,
// freeing memory when sigint or sigterm is caught
Server server;

void free_server(Server server) {
    for (int i = 0; i < server.gameAmount; i++) {
        GameProp prop = server.games[i];
        for (int j = 0; j < prop.instanceSize; j++) {
            struct Game instance = prop.instances[j];
            for (int k = 0; k < instance.playerCount; k++) {
                struct GamePlayer player = instance.players[k];
                fclose(player.toPlayer);
                fclose(player.fromPlayer);
                free(player.state.name);
            }
            free(instance.name);
            if (instance.playerCount > 0) {
                free(instance.players);
            }
            // pthread_join(prop.instanceThreads[j], NULL);
        }
        free(prop.instances);
        free(prop.instanceThreads);
    }
    free(server.games);
}

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

// void setup_connection(Connection *connection, int sock) {
//     connection->socket = sock;
//     connection->in = fdopen(sock, "w");
//     connection->out = fdopen(sock, "r");
//     connection->state = AUTHENCATING;
// }

pthread_mutex_t gamePropMutex = PTHREAD_MUTEX_INITIALIZER;

void *game_instance_thread(void *arg) {
    struct Game *g = (struct Game *) arg;
    int playerMax = *((int *) g->data);
    while(g->playerCount < playerMax) {}; // Wait until there is enough players.
    printf("Game started!\n");
    //printf("thread:game: %s running on thread: %i\n", g->name, (int)pthread_self());
    // printf("thread:game has %i players\n", g->playerCount);
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

// void remove_connection(Server *server, int index) {
//     GamePropConnection *newArr = malloc(sizeof(GamePropConnection)
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

// void handle_new_connection(Server *server, int sock) {
//     Connection connection;
//     setup_connection(&connection, sock);
//     if (!verifiy_connection(server, &connection)) {
//         free_connection(&connection);
//         return;
//     }
//     char *buffer;
//     read_line(connection.out, &buffer, 0);
//     if (strcmp(buffer, "reconnect") == 0) {
//         // Handle reconnect
//         free(buffer);
//         return;
//     }
//     GameProp game;
//     int index = index_of_game(server, buffer);
//     if (index == -1) {
//         //printf("setting up new game\n");
//         setup_game(&game, 5, buffer);
//         free(buffer);
//         add_game(server, game);
//         index = server->gameAmount - 1;
//         game = server->games[index];
//         //printf("game name: %s\n", game.name);
//         read_line(connection.out, &connection.name, 0);
//         add_connection(&server->games[index], connection);
//         pthread_create(&server->games[index].thread, NULL,
//                 play_game, (void *) &server->games[index]);
//     } else {
//         free(buffer);
//         game = server->games[index];
//         //printf("adding to existing game\n");
//         read_line(connection.out, &connection.name, 0);
//         add_connection(&server->games[index], connection);
//     }
// }

void setup_player(struct GamePlayer *player, int id) {
    struct Player state;
    initialize_player(&state, id);
    read_line(player->fromPlayer, &state.name, 0);
    player->state = state;
}

void play_game(GameProp *prop, int index) {
    int size = prop->instanceSize;
    pthread_mutex_lock(&gamePropMutex);
    prop->instanceThreads = realloc(prop->instanceThreads, sizeof(pthread_t) *
        (size));
    //printf("outsideThread: %s\n", prop->instances[index].name);
    prop->instances[index].data = (void *) &prop->playerMax;
    pthread_create(&prop->instanceThreads[size - 1], NULL, game_instance_thread,
            (void *) &prop->instances[index]);
    pthread_mutex_unlock(&gamePropMutex);
}

void create_new_game(GameProp *prop, struct GamePlayer *player,
        char *name) {
    printf("SETTING UP NEW GAME\n");
    struct Game instance = setup_instance(name);
    int index = prop->instanceSize;
    setup_player(player, instance.playerCount);
    add_player(&instance, player);
    add_instance(prop, instance);
    play_game(prop, index);
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
        }
    }
    return index;
}

void handle_connection(GameProp *prop, int sock) {
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
    int index = get_avaliable_game(prop, buffer);
    printf("game avaliable at %i\n", index);
    if (index == -1) {
        create_new_game(prop, &player, buffer);
    } else {
        free(buffer);
        add_to_existing_game(prop, &player, index);
    }
}

void *listen_thread(void *arg) {
    printf("LISTENER THREAD %i STARTED\n", (int)pthread_self());
    GameProp *prop = (GameProp *) arg;
    struct sockaddr_in in;
    socklen_t size = sizeof(in);
    int x = 0;
    while(1) {
        int sock = accept(prop->socket, (struct sockaddr *) &in, &size);
        if (sock == -1) {
            exit_with_error(FAILED_LISTEN);
        }
        handle_connection(prop, sock);
        if (x++ > 5) break;
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

void *game_thread(void *g) {
    GameProp *prop = (GameProp *) g;
    printf("GAMETHREAD %i LISTENING ON %s\n", (int) pthread_self(),prop->port);
    pthread_create(&prop->listenThread, NULL, listen_thread, (void *) prop);
    pthread_join(prop->listenThread, NULL);
    return NULL;
}

void start_server(Server *server) {
    for (int i = 0; i < server->gameAmount; i++) {
        pthread_create(&server->games[i].mainThread, NULL, game_thread,
                (void *) &server->games[i]);
    }
    for (int i = 0; i < server->gameAmount; i++) {
        pthread_join(server->games[i].mainThread, NULL);
    }
}

void setup_server(Server *server) {
    server->timeout = 0;
    server->gameAmount = 0;
    server->key = "12345";
}

void signal_handler(int sig) {
    switch(sig) {
        case(SIGINT):
            printf("SIGINT CAUGHT\n");
            for (int i = 0; i < server.gameAmount; i++) {
                GameProp prop = server.games[i];
                pthread_cancel(prop.listenThread);
                pthread_cancel(prop.mainThread);
                pthread_join(prop.listenThread, NULL);
                pthread_join(prop.mainThread, NULL);
            }
            free_server(server);
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
    server->games = malloc(sizeof(GameProp) * amount);
    server->gameAmount = amount;
    for (int i = 0; i < amount; i++) {
        server->games[i].port = ports[i];
        server->games[i].socket = get_socket(ports[i]);
        server->games[i].key = "12345";
        server->games[i].instanceSize = 0;
        server->games[i].instances = malloc(sizeof(struct Game));
        server->games[i].instanceThreads = malloc(0);
        server->games[i].playerMax = 2;
    }
}

int main(int argc, char **argv) {
    setup_signal_handler();
    check_args(argc, argv);
    load_keyfile(argv[1]);
    load_deckfile(argv[2]);
    load_statfile(argv[3]);
    char *ports[1] = {"3000"};
    setup_server(&server);
    setup_game_sockets(&server, ports, 1);
    start_server(&server);
    free_server(server);
}