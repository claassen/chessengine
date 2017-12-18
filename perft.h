#ifndef PERFT_H
#define PERFT_H

#include "game.h"

void perft(Game* game, int depth, move& moveMade);
void perftTestSuite(Game* game);
void perftDivide(Game* game, int depth);

#endif