#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.h"
#include "cmsg.h"



// returns -1 for negative numbers and 1 for positive
short sign(int x)
{
    return (x < 0) ? -1 : 1;
}

// Will this message type end the game
bool is_terminal(MType t)
{
    return ((t == HEOF) || (t == GAME_OVER) || (t == CERROR) || (t==ODISCO) || (t==OBADMSG));
}

// Does this message belong in the execute phase?
bool is_execute(MType t)
{
    switch (t) {
        case CHOOSE_H:
        case CHOOSE_LONG:
        case CHOOSE_SHORT:
        case MOVED_H:
        case MOVED_V:
        case TARGETED_L:
        case TARGETED_S:
        case LOOTED:
        case RECOVERED:
            return true;
        default:
            return false;
    }
}

// Submit an action
void send_action(unsigned char c, FILE* out)
{
    fprintf(out, "play%c\n", c);
    fflush(out);
}

// Submit a choice for h movement
void send_hchoice(int v, FILE* out)
{
    char c = (v > 0) ? '+' : '-';
    fprintf(out, "sideways%c\n", c);
    fflush(out);
}

// Submit a long target choice
void send_long_choice(unsigned char c, FILE* out)
{
    fprintf(out, "target_long%c\n", (c != CLIENT_NO_TARGET) ? (c + 'A') : '-');
    fflush(out);
}

// Submit a short target choice
void send_short_choice(unsigned char c, FILE* out)
{
    fprintf(out, "target_short%c\n", (c != CLIENT_NO_TARGET) ? (c + 'A') : '-');
    fflush(out);
}

// Record the fact than a player has submitted an action
void record_order(Game* g, CMsg m, FILE* log)
{
    if (log) {
        fprintf(log, "%s Ordered %c\n", g->players[m.subject].name, m.order);
        fflush(log);
    }
}

// Record the fact that a player has moved horizontally
bool record_h(Game* g, CMsg m, FILE* log)
{
    // is it legal?
    int max = 1;
    if ((m.value == 0) || (m.value < -max) || (m.value > max)) {
        return false;
    }
    // would it move out of bounds?
    if ((m.value + g->players[m.subject].x >= g->width)
            || (m.value + g->players[m.subject].x < 0)) {
        return false;
    }
    g->players[m.subject].x += m.value;
    if (log) {
        fprintf(log, "%s moved to %d/%d\n", g->players[m.subject].name,
                g->players[m.subject].x, g->players[m.subject].y);
        fflush(log);
    }
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = false;
    }
    return true;
}

// Record the fact that a player has moved vertically
bool record_v(Game* g, CMsg m, FILE* log)
{
    g->players[m.subject].y = (g->players[m.subject].y + 1) % 2;
    if (log) {
        fprintf(log, "%s moved %s to %d/%d\n", g->players[m.subject].name,
                (g->players[m.subject].y ? "up" : "down"),
                g->players[m.subject].x, g->players[m.subject].y);
        fflush(log);
    }
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = false;
    }

    return true;
}

// Record the fact that a player has executed long
bool record_long(Game* g, CMsg m, FILE* log)
{
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = true;
    }
    // legal target?
    if (m.missingObject) {
        // this is not actually checking to see if they could have 
        // targeted someone
        if (log) {
            fprintf(log, "%s missed\n", g->players[m.subject].name);
            fflush(log);
        }
        return true;
    }
    if (g->players[m.subject].y != g->players[m.object].y) {
        return false;
    }
    if (g->players[m.subject].x == g->players[m.object].x) {    // too close
        return false;
    }
    if (g->players[m.subject].y) {      // line of sight shot
        int dist = g->players[m.subject].y - g->players[m.object].y;
        // we need to check if there is anyone closer in that direction
        for (int i = 0; i < g->pCount; ++i) {
            if (g->players[i].y == 0) {
                continue;
            }
            if ((abs(g->players[i].x - g->players[m.object].x) < abs(dist))
                    && (sign(dist) ==
                    sign(g->players[i].x - g->players[m.object].x))) {
                return false;   // there is something closer in that direction
            }
        }
    } else {                    // lower level
        if (abs(g->players[m.subject].x - g->players[m.object].x) != 1) {
            return false;
        }
    }
    g->players[m.object].hits++;
    if (log) {
        fprintf(log, "%s long %s\n", g->players[m.subject].name,
                g->players[m.object].name);
        fflush(log);
    }
    return true;
}

// Record the fact that a player has executed a short
bool record_short(Game* g, CMsg m, FILE* log)
{
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = true;
    }
    if (m.missingObject) {
        // this is not actually checking to see if they could have 
        // targeted someone
        if (log) {
            fprintf(log, "%s missed\n", g->players[m.subject].name);
            fflush(log);
        }
        return true;
    }
    // so we need to have them drop a loot
    if (g->players[m.object].loot) {
        g->players[m.object].loot--;
        g->loot[2 * g->players[m.object].x + g->players[m.object].y]++;
    }
    if (log) {
        fprintf(log, "%s shorted\n", g->players[m.subject].name);
        fflush(log);
    }
    return true;
}

// Record the fact that a player tried to loot
bool record_looted(Game* g, CMsg m, FILE* log)
{
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = false;
    }
    if (g->loot[2 * g->players[m.subject].x + g->players[m.subject].y] > 0) {
        g->loot[2 * g->players[m.subject].x + g->players[m.subject].y]--;
        g->players[m.subject].loot++;
        if (log) {
            fprintf(log, "%s looted\n", g->players[m.subject].name);
            fflush(log);
        }
    } else {
        if (log) {
            fprintf(log, "%s failed to loot\n", g->players[m.subject].name);
            fflush(log);
        }
    }
    return true;
}

// Record the fact that a player dried out
bool record_recover(Game* g, CMsg m, FILE* log)
{
    if (m.subject == g->ci->myID) {
        g->ci->targetLastTurn = false;
    }
    g->players[m.subject].hits = 0;
    if (log) {
        fprintf(log, "%s dried out\n", g->players[m.subject].name);
        fflush(log);
    }
    return true;
}

/* A note on state machine implementation:
   eg(1):
   while (x=getInput()) {
      switch 
          State1: .....
          State2: .....
       
   }
   
   The above is simplest and fits the typical model.
   (For each input, a state transition happens.)
   (This would have worked for play_game).
   
   This can be a bit tricky to get right if you need to
   jump to another state without consuming input.
   eg(2):    
   State4: 
    if x==A:
          do4Stuff()
          state=4
    if x==B:
          state=5
          do5Stuff()
   State5:
    if x==A:
        state=4
        do4Stuff()
    if x==B
        do5Stuff()
        
   That is, if you get some input, immediately switch to another state, 
and do that state's action.
   It can be written as above (2), but if do5Stuff is a lot of 
code/options to consider, you get repeated code and long functions 
(asking for bugs). 
   What you want to be able to do would be:
   State4: 
    if x==A:
          do4Stuff()
    if x==B:
          state=5
   State5:
    if x==A:
        state=4
    if x==B
        do5Stuff()
        
    But if you get a new input every time through the loop, 
you've skipped actions.
    So instead, you can do what I've done here which is to explicitly 
get new input when necessary.
    Pros: more flexible.
    Cons: you need to remember to get new input.
    
*/

// Play a game as client
CResult play_client_game(Game* g, FILE* in, FILE* out, FILE* log)
{
    CMsg msg = get_message(in, g->pCount);
    while (!is_terminal(msg.type)) {
        switch (g->state) {
            case START:
                if (msg.type != ROUND) {
                    return C_COMERROR;
                }
                g->state = ORDERS;
                msg = get_message(in, g->pCount);
                break;
            case ORDERS:
                if (msg.type == TURN) {
                    while (true) {
                        unsigned char ord = g->ci->getTurn(g);
						if (ord == CLIENT_EOF_INDICATOR) {
							return C_EOF;
						}
                        send_action(ord, out);
                        CMsg auth = get_message(in, g->pCount);
                        if (auth.type == GAME_OVER) {
                            return C_OK;
                        }
                        if (auth.type == ACK) {
                            break;
                        } else if (auth.type != NACK || !g->ci->reprompt) {
                            return C_COMERROR;
                        }
                    }
                } else if (msg.type == ORDERED) {
					if (g->ci->recordOrder) {
						g->ci->recordOrder(g, msg, log);
					}
                } else if (msg.type == EXECUTE_PHASE) {
                    g->state = EXECUTE;
                } else {
                    return C_COMERROR;
                }
                msg = get_message(in, g->pCount);
                break;
            case EXECUTE:
                if (msg.type == ROUND) {
                    game_summary(g, stdout); 
                    g->state = ORDERS;
                    msg = get_message(in, g->pCount);
                    break;
                }
                if (!is_execute(msg.type)) {
                    return C_COMERROR;
                }
                if (!g->ci->processExecute(g, msg, out, log)) {
					if (feof(stdin)) {
						return C_EOF;
					}
                    return C_COMERROR;
                }
                msg = get_message(in, g->pCount);
                break;
        }
    }
    if (g->gameOver || (msg.type == GAME_OVER)) {
        game_summary(g, stdout);  
        print_winners(g, stdout);
        return C_OK;
    }
    print_winners(g, stdout);    
    if (msg.type == OBADMSG) {
        return C_BADOTHER;
    }
    if (msg.type == ODISCO) {
        return C_CDISCO;
    }
    if (msg.type == CERROR) {
        return C_COMERROR;
    }
    if (msg.type == HEOF) {
        return C_SDISCO;
    }
    return C_COMERROR;
}


void game_summary(Game* g, FILE* out) {
    for (int i=0;i<g->pCount;++i) {
        Player* p=&(g->players[i]);
        fprintf(out, "%c: %s:%d/%d $=%d,h=%d\n", p->id, p->name, p->x, p->y, p->loot, p->hits);
    }
    for (int i=0;i<g->width;++i) {
        fprintf(out, "Carriage %d: $=%d,$=%d\n", i, g->loot[2*i], g->loot[2*i+1]);
    }
    fflush(out);
}

void print_winners(Game* g, FILE* out) {
    int high=0;
    for (int i=0;i<g->pCount;++i) {
        if (g->players[i].loot>high) {
            high=g->players[i].loot;
        }
    }
    fprintf(out, "Winner(s):");    
    bool first=true;
    for (int i=0;i<g->pCount;++i) {
        if (g->players[i].loot==high) {
            if (!first) {
                fputc(',', out);
            }
            fprintf(out, "%s", g->players[i].name);
            first=false;
        }
    }
    fputc('\n', out);
    fflush(out);
}
