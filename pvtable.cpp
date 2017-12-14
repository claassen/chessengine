#include "pvtable.h"

struct pv_entry {
    size_t key;
    move move;
};

static pv_entry* pvTable;
static int pvTableSize;

void initPvTable(int sizeInBytes) {
    pvTableSize = sizeInBytes / sizeof(pv_entry);
    pvTable = new pv_entry[pvTableSize];
}

void addPvMove(const gameState& gameState, const move& move) {
    int index = gameState.hashCode % pvTableSize;

    pvTable[index] = {
        .key = gameState.hashCode,
        .move = move
    };
}

move getPvMove(const gameState& gameState) {
    int index = gameState.hashCode % pvTableSize;

    pv_entry entry = pvTable[index];

    if(entry.key == gameState.hashCode) {
        return entry.move;
    }
    
    return NO_MOVE;
}