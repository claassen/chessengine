#ifndef SEARCH_H
#define SEARCH_H

#include <climits>

#include "game.h"

const int alphaBeta(Game* game, move& mv, int depth, int alpha, int beta, Colour turn, int ply, volatile bool* stop);

#endif