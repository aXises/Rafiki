#include "utils.h"

// Calculate distance to closest other player on each side
// players on a different level or in the same cell don't count
void closest(Game* g, int plnum, int* left, int* right)
{
    *left = g->width;
    *right = g->width;
    Player* p = &(g->players[plnum]);
    for (int i = 0; i < g->pCount; ++i) {
        if (i != plnum) {
            Player* o = &(g->players[i]);
            if ((o->y != p->y) || (o->x == p->x)) {
                continue;
            }
            if (o->x < p->x) {
                int n = p->x - o->x;
                if (n < *left) {
                    *left = n;
                }
            } else {
                int n = o->x - p->x;
                if (n < *right) {
                    *right = n;
                }
            }
        }
    }
}

// Are there any valid targets for short by this player?
//  legal targets have true set in the target array
bool short_targets(Game* g, unsigned char pl, bool* targets)
{
    unsigned int x = g->players[pl].x;
    unsigned int y = g->players[pl].y;
    bool found = false;
    for (int i = 0; i < g->pCount; ++i) {
        if ((i != pl) && (g->players[i].x == x) && (g->players[i].y == y)) {
            found = true;
            targets[i] = true;
        } else {
            targets[i] = false;
        }
    }
    return found;
}

// Are there any valid targets for long by this player?
//  legal targets have true set in the target array
bool long_targets(Game* g, unsigned char pl, bool* targets)
{
    unsigned int x = g->players[pl].x;
    unsigned int y = g->players[pl].y;
    bool found = false;
    if (y == 0) {
        for (int i = 0; i < g->pCount; ++i) {
            if ((i != pl)
                    && ((g->players[i].x == x + 1) 
                    || (g->players[i].x + 1 == x))
                    && (g->players[i].y == 0)) {
                found = true;
                targets[i] = true;
            } else {
                targets[i] = false;
            }
        }
    } else {
        int left, right;
        closest(g, pl, &left, &right);
        if ((left == g->width) && (right == g->width)) {
            for (int i = 0; i < g->pCount; ++i) {
                targets[i] = false;
            }
            return false;
        }
        for (int i = 0; i < g->pCount; ++i) {
            targets[i] = false;
            if (g->players[i].x == x) {
                continue;
            }
            if (g->players[i].x < x) {
                targets[i] = (g->players[i].x + left == x);
            } else {
                targets[i] = (g->players[i].x == x + right);
            }
            if (targets[i]) {
                found = true;
            }
        }
    }
    return found;
}

// true if the char encodes an order
bool is_order(unsigned char c)
{
    switch (c) {
        case 'v':
        case 'h':
        case 'l':
        case 's':
        case '$':
		case 'r':
            return true;
        default:
            return false;
    }
}
