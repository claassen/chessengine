#ifndef SEARCH_H
#define SEARCH_H

#include "game.h"

const int quiesce(Game* game, int alpha, int beta, volatile bool* stop);
const int alphaBeta(Game* game, move& mv, int depth, int alpha, int beta, int ply, volatile bool* stop);

#endif