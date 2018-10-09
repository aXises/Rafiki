#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"
#include "cmsg.h"
#include "smsg.h"

// send message to all players in the game
void send_msg_to_all(Game* g, CMsg m)
{
    char buff[20];
    encode_message(buff, m);
    for (int i = 0; i < g->pCount; ++i) {
        fprintf(g->players[i].to, "%s\n", buff);
        fflush(g->players[i].to);
    }
}

// send message to a particular player
void send_msg_to_player(Player* p, CMsg m)
{
    char buff[20];
    encode_message(buff, m);
    fprintf(p->to, "%s\n", buff);
    fflush(p->to);
}

// playerNumber is the current player
// Note that this only records orders, it does not execute them
HResult do_turn(Game* g, int playerNumber)
{
	if (g->players[playerNumber].hits >= g->dryOutLimit) {
		g->players[playerNumber].order = 'r';
	} else {
		CMsg ms;
		memset(&ms, 0, sizeof(ms));
		ms.type = TURN;
		send_msg_to_player(&g->players[playerNumber], ms);
		// now we need to get a message
		SMsg sm = get_serv_message(g->players[playerNumber].from);
		if (sm.type == CEOF) {
			return H_CDISCO;
		}
		if ((sm.type != PLAY) || (!is_order(sm.order))) {
			return H_PROTOERR;
		}
		g->players[playerNumber].order = sm.order;
	}
    return H_OK;
}

// execute vertical order for a player
HResult do_vert(Game* g, int playerNumber)
{
    Player* p = &(g->players[playerNumber]);
    CMsg c;
    p->y = (p->y) ? 0 : 1;
    c.type = MOVED_V;
    c.subject = p->num;
    g->sendMsgToAll(g, c);
    return H_OK;
}

// execute horizontal order for a player
HResult do_horiz(Game* g, int playerNumber)
{
    CMsg c;
    c.type = CHOOSE_H;
    Player* p = &(g->players[playerNumber]);
    send_msg_to_player(&g->players[playerNumber], c);
    SMsg sm;
	while(true) {
		sm = get_serv_message(g->players[playerNumber].from);
        	if (sm.type == CEOF) {
            		return H_CDISCO;
        	}
		if (sm.type != HMOVE) {
			return H_PROTOERR;	
		}
		// now we need to check if the move is legal
		if ((sm.value != -1) && (sm.value != 1)) {
			return H_PROTOERR;
		}
		memset(&c, 0, sizeof(CMsg));
		if (((sm.value == -1) && (p->x == 0)) || ((sm.value == 1) && (p->x == g->width - 1))) {
			c.type=NACK;
			send_msg_to_player(&g->players[playerNumber], c);
			continue;
		}
		c.type=ACK;
		send_msg_to_player(&g->players[playerNumber], c);
        break;
	} 
    p->x += sm.value;
    c.type = MOVED_H;
    c.subject = p->num;
    c.value = sm.value;
    g->sendMsgToAll(g, c);
    return H_OK;
}

// handle long operation with missing target
HResult missing_long(Game* g, int playerNumber) {
    Player* p = &(g->players[playerNumber]);
    bool* b =  malloc(sizeof(bool)*g->pCount);
    if (long_targets(g, playerNumber, b)) {
        free(b);
        return H_BADMOVE;
    }
    free(b);        // yes there are nicer ways to do this
    CMsg c;
    memset(&c, 0, sizeof(CMsg));  
    c.type = ACK;
    send_msg_to_player(&g->players[playerNumber], c);
    c.type = TARGETED_L;
    c.subject = p->num;
    c.missingObject = true;
    g->sendMsgToAll(g, c);
    return H_OK;
}

// This will be harder because you need to know if there is someone closer
HResult do_longr(Game* g, int playerNumber)
{
    CMsg c;
    memset(&c, 0, sizeof(CMsg));
    c.type = CHOOSE_LONG;
    Player* p = &(g->players[playerNumber]);
    send_msg_to_player(&g->players[playerNumber], c);
	SMsg sm;
	Player* o = 0;
	while (true) {
		sm = get_serv_message(g->players[playerNumber].from);
		if (sm.type == CEOF) {
		    return H_CDISCO;
		}        
		if (sm.type != LTARGET) {
			return H_PROTOERR;
		}
		if (sm.missingObject) {
			HResult res = missing_long(g, playerNumber);
			if (res == H_BADMOVE) {
				memset(&c, 0, sizeof(CMsg));
				c.type = NACK;
				send_msg_to_player(&g->players[playerNumber], c);				
				continue;
			}
			return res;
		} else {
			if (sm.object >= g->pCount) {    // it will never be legal to 
											 // target a non-existant player
				memset(&c, 0, sizeof(CMsg));
				c.type = NACK;
				send_msg_to_player(&g->players[playerNumber], c);
				continue;
			}
			o = &(g->players[sm.object]);
			if ((p == o) ||         // can't target self 
					(p->y != o->y) || (p->x == o->x)) {// must be on the same  
													   // level but not same cell
				memset(&c, 0, sizeof(CMsg));
				c.type = NACK;
				send_msg_to_player(&g->players[playerNumber], c);
				continue;  
			}
			if (p->y == 0) {
				if ((p->x != o->x + 1) && (o->x != p->x + 1)) {
					memset(&c, 0, sizeof(CMsg));
					c.type = NACK;
					send_msg_to_player(&g->players[playerNumber], c);
					continue;  
				}
			} else {  // we need to find out if there is anyone closer
				int dist = o->x - p->x;
				int left, right;
				closest(g, playerNumber, &left, &right);
				if (((dist < 0) && (left < -dist)) || ((dist < 0) && (right < dist))){
					memset(&c, 0, sizeof(CMsg));
					c.type = NACK;
					send_msg_to_player(&g->players[playerNumber], c);
					continue;  
				}
			}
		}
		memset(&c, 0, sizeof(CMsg));
		c.type = ACK;
		send_msg_to_player(&g->players[playerNumber], c);		
		break;
	}
	o->hits++;
	c.type = TARGETED_L;
	c.subject = p->num;
	c.object = o->num;
	g->sendMsgToAll(g, c);
	return H_OK;
}

// execute short order for a player
HResult do_short(Game* g, int playerNumber)
{
    CMsg c;
    c.type = CHOOSE_SHORT;
    Player* p = &(g->players[playerNumber]);
    send_msg_to_player(&g->players[playerNumber], c);
    SMsg sm;
	Player* o = 0;
	while (true) {
		sm = get_serv_message(g->players[playerNumber].from);
		if (sm.type == CEOF) {
		    return H_CDISCO;
		}        
		if (sm.type != STARGET) {
			return H_PROTOERR;
		}
		// need to check if there is any other target
		if (sm.missingObject) {
			for (int i = 0; i < g->pCount; ++i) {
				if ((i != playerNumber) && (p->x == g->players[i].x)
						&& (p->y == g->players[i].y)) {
					memset(&c, 0, sizeof(CMsg));
					c.type = NACK;
					send_msg_to_player(&g->players[playerNumber], c);
					continue;
				}
			}
		} else {
			if (sm.object >= g->pCount) { 
				memset(&c, 0, sizeof(CMsg));
				c.type = NACK;
				send_msg_to_player(&g->players[playerNumber], c);
				continue;

			}
			o = &(g->players[sm.object]);
			if ((o == p) || (o->y != p->y) || (o->x != p->x)) {
				memset(&c, 0, sizeof(CMsg));
				c.type = NACK;
				send_msg_to_player(&g->players[playerNumber], c);
				continue;
			}
		}
		memset(&c, 0, sizeof(CMsg));
		c.type = ACK;
		send_msg_to_player(&g->players[playerNumber], c);
		break;
	}
    memset(&c, 0, sizeof(CMsg));
    c.type = TARGETED_S;
    c.subject = playerNumber;
	
	if (sm.missingObject) {
		c.missingObject = true;
	} else {
		if (o->loot > 0) {
			o->loot--;
			g->loot[2 * o->x + o->y]++;
		}
		c.object = sm.object;
    }
    g->sendMsgToAll(g, c);
    return H_OK;
}

// execute loot order for a player
HResult do_loot(Game* g, int playerNumber)
{
    if (g->loot[g->players[playerNumber].x * 2 + g->players[playerNumber].y] > 0) {
        g->loot[g->players[playerNumber].x * 2 + g->players[playerNumber].y]--;
        g->players[playerNumber].loot++;
		g->serverLootHook(g, playerNumber);
    }
    CMsg c;
    c.type = LOOTED;
    c.subject = playerNumber;
    g->sendMsgToAll(g, c);
    return H_OK;
}

// execute dry out order for a player
HResult do_recover(Game* g, int playerNumber)
{
    g->players[playerNumber].hits = 0;
    CMsg c;
    c.type = RECOVERED;
    c.subject = playerNumber;
    g->sendMsgToAll(g, c);
    return H_OK;
}

// playerNumber is the current player
HResult do_order(Game* g, int playerNumber)
{
    HResult res = H_OK;
    switch (g->players[playerNumber].order) {
        case 'r':
            res = do_recover(g, playerNumber);
            break;
        case 'v':
            res = do_vert(g, playerNumber);
            break;
        case 'h':
            res = do_horiz(g, playerNumber);
            break;
        case 'l':
            res = do_longr(g, playerNumber);
            break;
        case 's':
            res = do_short(g, playerNumber);
            break;
        case '$':
            res = do_loot(g, playerNumber);
            break;
        default:
            return H_PROTOERR;
    }
    return res;
}
