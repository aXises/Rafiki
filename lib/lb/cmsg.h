

#ifndef CMSG_H
#define CMSG_H
#include <stdbool.h>
#include <stdio.h>

/* Describes messages from server to client */


typedef enum {
    ACK,        // a yes mesage
    NACK,        // a no message    
    HEOF,       // end of file from hub
    CERROR,	// Message could not be decoded
    OBADMSG,	// Another player sent the server a bad message
    ODISCO,     // Another player disconnected
    GAME_OVER,         // declare the winner    
    ROUND,          // begin a new round
    EXECUTE_PHASE,        // move to execute phase
    TURN,          // It is your turn
    ORDERED,         // someone submitted an order
// below here are execute messages    
    CHOOSE_H,
    CHOOSE_LONG,     // Choose long range target
    CHOOSE_SHORT,    // Choose short range target
    MOVED_H,        // player moved horizontally
    MOVED_V,        // player moved vertically
    TARGETED_L,     // player targeted long
    TARGETED_S,     // player targeted short
    LOOTED,         // player looted
    RECOVERED       // player dried out
} MType;

   // message from server to client
typedef struct {
    MType type;
    int value;
    unsigned char subject;  // who is doing    [number not char]
    unsigned char object;   // who is done to  [number not char]
    bool missingObject;    // true if object is -
    unsigned char order;
} CMsg;

   // Get a message from the sever
CMsg get_message(FILE* f, int pCount);

   // Encode a message to clients into buffer
void encode_message(char* buff, CMsg);

const char* mtype_to_str(MType m);

#endif
