#ifndef PVTABLE_H
#define PVTABLE_H

#include "game.h"

enum ScoreFlag {
    SCORE_NONE = 0,
    SCORE_ALPHA,
    SCORE_EXACT,
    SCORE_BETA
};

struct pv_entry {
    unsigned long long key;
    move move;
    int score;
    int depth;
    ScoreFlag scoreFlag;

    bool operator==(const pv_entry& rhs) {
        return key == rhs.key &&
            score == rhs.score &&
            depth == rhs.depth &&
            scoreFlag == rhs.scoreFlag &&
            move == rhs.move;
    }

    bool operator!=(const pv_entry& rhs) {
        return !(*this == rhs);
    }
};

#define NO_PV_ENTRY (pv_entry{.key = 0, .move = NO_MOVE, .score = 0, .depth = 0, .scoreFlag = SCORE_NONE})

void initPvTable(int sizeInBytes);
void addPvMove(const gameState& gameState, const move& m, int score, int depth, ScoreFlag scoreFlag);
const pv_entry getPvEntry(const gameState& gameState);
void getPvLine(Game* game, std::vector<move>& pvMoves, int depth);
void printPvStatistics();

#endif