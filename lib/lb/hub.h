#ifndef HUB_H
#define HUB_H

#include "types.h"
#include "cmsg.h"

   // send message to all players in the game
void send_msg_to_all(Game* g, CMsg m);
   // send message to a particular player
void send_msg_to_player(Player* p, CMsg m);
   // get orders from a player
HResult do_turn(Game* g, int plnum);
   // execute vertical order for a player
HResult do_vert(Game* g, int plnum);
   // execute horizontal order for a player
HResult do_horiz(Game* g, int plnum);
   // execute long order for a player
HResult do_longr(Game* g, int plnum);
   // execute short order for a player
HResult do_short(Game* g, int plnum);
   // execute loot order for a player
HResult do_loot(Game* g, int plnum);
   // execute dry out order for a player
HResult do_recover(Game* g, int plnum);
   // overall function to carry out orders
HResult do_order(Game* g, int plnum);
#endif
