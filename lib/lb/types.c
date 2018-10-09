#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "hub.h"

// doesn't do anything
void dummy_loot(Game* g, int plnum) {
}

// returns an allocated game struct
Game* create_game(int pCount, unsigned int width, unsigned int seed,
        bool server)
{
    Game* res = malloc(sizeof(Game));
    res->pCount = pCount;
    res->seed = seed;
    res->players = malloc(sizeof(Player) * pCount);
    for (int i = 0; i < pCount; ++i) {
        res->players[i].id = (unsigned char)(i + 'A');
        res->players[i].num = (unsigned char)i;
        res->players[i].name = 0;
        res->players[i].loot = 0;
        res->players[i].hits = 0;
        res->players[i].x = i % width;
        res->players[i].y = 0;
        res->players[i].order = 0;
        res->players[i].to = 0; // null for client use
        res->players[i].from = 0;       // null for client use    
    }
    res->ci = 0;
    if (!server) {
        res->ci = malloc(sizeof(ClientInfo));
        memset(res->ci, 0, sizeof(ClientInfo));
        // clients should set up the other members themselves
    }
    res->seed = seed;
    res->width = width;
    res->loot = malloc(sizeof(unsigned int) * width * 2);
    memset(res->loot, 0, sizeof(unsigned int) * width * 2);
    res->numRounds = 15;
    res->dryOutLimit = 3;
    res->state = START;
	res->custom = 0;
	res->freeCustom = 0;
	res->serverLootHook = dummy_loot;
	res->sendMsgToAll = send_msg_to_all;
    res->gameOver = false;
    return res;
}

// deallocates a game and associated memory
void free_game(Game* g)
{
    for (int i = 0; i < g->pCount; ++i) {
        if (g->players[i].to) {
            fclose(g->players[i].to);
            g->players[i].to = 0;
        }
        if (g->players[i].from) {
            fclose(g->players[i].from);
            g->players[i].from = 0;
        }
        if (g->players[i].name) {
            free(g->players[i].name);
            g->players[i].name = 0;
        }
    }
    free(g->players);
    if (g->ci) {
        free(g->ci);
        g->ci = 0;
    }
    free(g->loot);
	if (g->custom && g->freeCustom) {
		g->freeCustom(g->custom);
	}
    free(g);
}
