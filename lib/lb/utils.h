#ifndef UTILS_H
#define UTILS_H

#include "types.h"

/* helper rountines for move processing */

  // Calculate distance to closest other player on each side
  // players on a different level or in the same cell don't count
void closest(Game* g, int plnum, int* left, int* right);
  // Are there any legal targets for short by this player?
  //   legal targets have true set in the targets array
bool short_targets(Game* g, unsigned char player, bool* targets);
  // same as above but for long
bool long_targets(Game* g, unsigned char player, bool* targets);
  // true if the char encodes an order
bool is_order(unsigned char c);
#endif
