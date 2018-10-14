#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#include <stdint.h>
#include <game.h>
#include <util.h>

#define LOCALHOST "127.0.0.1"

typedef struct {
    char *name;
    int socket;
    int port;
    FILE *in;
    FILE *out;
    struct Player player;
} Connection;

void load_keyfile(char *);
void send_message(FILE *, char *, ...);

#endif

