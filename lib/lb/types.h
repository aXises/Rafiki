#ifndef A4TYPES_H
#define A4TYPES_H
#include <stdbool.h>
#include <stdio.h>
#include "cmsg.h"

#define CLIENT_EOF_INDICATOR 255
#define CLIENT_NO_TARGET 254


typedef enum {
    START,
    ORDERS,
    EXECUTE
} RoundState;

// Result for server functions
typedef enum {
    H_OK = 0,
    H_USAGE = 1,
    H_BADPCHAR = 2,
    H_BADSEED = 3,
    H_TIMEOUT = 4,
    H_BADPORT = 5,	
    H_LISTEN = 6,
	H_BADMOVE = 10,
    H_PROTOERR = 11,
    H_CDISCO = 12		// client disconnected
} HResult;

typedef struct {
    unsigned char id;           // character for representation purposes
    unsigned char num;          // for lookup purposes
    char* name;
    int loot;
    int hits;
    int x, y;
    char order;
    FILE* to;                   // null for client use
    FILE* from;                 // null for client use
    bool disconnected;		// use this as you like
} Player;

struct ClientInfo;

typedef struct Game {
    Player* players;
    int pCount;
    struct ClientInfo* ci;     // null for server use
    unsigned int seed;
    unsigned int width;
    unsigned int* loot;         // note: 1D so to find loot use loot[2*x+y]
    int numRounds;              // rounds in the game
    int dryOutLimit;            // >= this will make player not have a turn    
    RoundState state;
    void* custom;
    void (*freeCustom)(void*);	// function to clean up custom
    void (*serverLootHook)(struct Game*, int);	// called when a player sucessfully loots (2nd param is player number)	
    void (*sendMsgToAll)(struct Game*, CMsg);	// called to send a message to all players in a game
    bool gameOver;
} Game;

/* Some of the functions listed in this struct may need to return sentinels:
     CLIENT_EOF_INDICATOR  will be returned if there is no valid value 
                           because of EOF
	 CLIENT_NO_TARGET      will be returned if there is no target to choose (-)
*/
typedef struct ClientInfo {
    unsigned char (*getTurn)(Game*);	
		// function to call to get the choice of move
		// (that is, the player's order - not who to target)

    int (*getHChoice)(Game*);			// function to call to determine which direction to move. 
    unsigned char (*getLongChoice)(Game*);	// call to determine who to "long". 
    unsigned char (*getShortChoice)(Game*);	// call to determine who to "short". 
    void (*recordOrder)(Game*, CMsg, FILE*);	// called in response to an 
				// ordered message. The FILE* is where mal's
				// extra output will be sent
    bool (*processExecute)(Game* g, CMsg msg, FILE* out, FILE* log);	
			// called to handle an 
			// execute phase message
			// out is a stream to the server
			// log is where mal's extra output will be sent

    int myID;               // player number
    bool targetLastTurn;    // did this player short or long last turn?
    bool reprompt;          // if false and server rejects an answer, treat as comserror
    int reconID;
    FILE* toServer;
    FILE* fromServer;    
} ClientInfo;

// Results for client functions
typedef enum {
    C_OK = 0,
    C_USAGE = 1,
    C_BADPCHAR = 2,
    C_BADCONNECT = 3,
    C_BADAUTH = 4,
    C_BADNAME = 5,
    C_BADRECON = 6,
    C_SDISCO = 7,
    C_CDISCO = 8,
    C_COMERROR = 9,
    C_BADOTHER = 10, 
	C_EOF = 11			// we got EOF when we needed input from user
} CResult;

// return initialised game structure
// pCount is number of players, width is number of carriages
// seed is as defined in spec
// server is false for client use (controls which sub structs
//   are allocated
Game* create_game(int pCount, unsigned int width, unsigned int seed,
        bool server);

// deallocate game and any linked memory
void free_game(Game* g);

#endif
