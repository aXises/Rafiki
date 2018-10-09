
#ifndef SMSG_H
#define SMSG_H

#include <stdbool.h>
#include <stdio.h>

/* Deals with messages from client to server */


  // Messages from client TO server/hub
typedef enum {
    SERROR,
    CEOF,
    PLAY,
    HMOVE,
    STARGET,
    LTARGET
} SMsgType;

  // stores a decoded message from client
typedef struct {
    SMsgType type;
    int value;              // used for hmoves
    unsigned char object;   // numeric not char form
    bool missingObject;    // an object is required but there isn't one
    unsigned char order;    // action the player chose
} SMsg;

  // read message from specified stream
SMsg get_serv_message(FILE* f);

  // return human readable name for message type enum value
const char* smtype_string(SMsgType t);

#endif
