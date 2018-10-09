
#ifndef PLAYER_COMMON_H
#define PLAYER_COMMON_H

#include <stdio.h>
#include "types.h"

/* Main client game driver and some helper routines */


  // Record the fact than a player has submitted an action
void record_order(Game* g, CMsg m, FILE* log);
  // Record the fact that a player has moved horizontally
bool record_h(Game* g, CMsg m, FILE* log);

  // Record the fact that a player has moved vertically
bool record_v(Game* g, CMsg m, FILE* log);
  // Record the fact that a player has executed long
bool record_long(Game* g, CMsg m, FILE* log);
  // Record the fact that a player has executed a short
bool record_short(Game* g, CMsg m, FILE* log);
  // Record the fact that a player tried to loot
bool record_looted(Game* g, CMsg m, FILE* log);
  // Record the fact that a player dried out
bool record_recover(Game* g, CMsg m, FILE* log);



// Submit an action
void send_action(unsigned char c, FILE* out);

// Submit a choice for h movement
void send_hchoice(int v, FILE* out);

// Submit a long target choice
void send_long_choice(unsigned char c, FILE* out);

// Submit a short target choice
void send_short_choice(unsigned char c, FILE* out);

// play game as client
CResult play_client_game(Game* g, FILE* in, FILE* out, FILE* log);

void game_summary(Game* g, FILE* out);
void print_winners(Game* g, FILE* out);

#endif
