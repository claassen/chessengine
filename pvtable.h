#ifndef PVTABLE_H
#define PVTABLE_H

#include "game.h"

void initPvTable(int sizeInBytes);
void addPvMove(const gameState& gameState, const move& move);
move getPvMove(const gameState& gameState);

#endif