#include "pvtable.h"
#include "utils.h"
#include "debug.h"

static pv_entry* pvTable;
static int pvTableSize;

static unsigned long long overwrites = 0;
static unsigned long long collisions = 0;
static unsigned long long hits = 0; 
static unsigned long long misses = 0;

void initPvTable(int sizeInBytes) {
    pvTableSize = sizeInBytes / sizeof(pv_entry);
    pvTable = new pv_entry[pvTableSize];

    for(int i = 0; i < pvTableSize; i++) {
        pvTable[i] = NO_PV_ENTRY;
    }
}

void addPvMove(const gameState& gameState, const move& m, int score, int depth, ScoreFlag scoreFlag) {
    int index = gameState.hashCode % pvTableSize;

    pv_entry existingEntry = pvTable[index];

    if(existingEntry.key != 0) {
        //Existing entry at index

        if(existingEntry.key == gameState.hashCode) {
            //Writing new values for same position key
            
            if(existingEntry.depth <= depth) {
                //New value has greater depth (more accurate score)
                overwrites++;    
            }
            else {
                //New value has lesser depth, don't overwrite
                return;
            }
        }
        else {
            //Hash key collision, overwrite
            collisions++;
        }
    }

    pvTable[index] = {
        .key = gameState.hashCode,
        .move = m,
        .score = score,
        .depth = depth,
        .scoreFlag = scoreFlag
    };
}

const pv_entry getPvEntry(const gameState& gameState) {
    int index = gameState.hashCode % pvTableSize;

    pv_entry entry = pvTable[index];

    if(entry.key == gameState.hashCode) {
        hits++;
        return entry;
    }
    
    misses++;

    return NO_PV_ENTRY;
}

void getPvLine(Game* game, std::vector<move>& pvMoves, int depth) {
    if(depth == 0) {
        return;
    }

    std::vector<move> availableMoves;
    game->generateMoves(game->currentState.turn, availableMoves, false);

    pv_entry pvEntry = getPvEntry(game->currentState);

    if(pvEntry.move == NO_MOVE) {
        return;
    }

    if(std::find(availableMoves.begin(), availableMoves.end(), pvEntry.move) == availableMoves.end()) {
        return;
    }

    pvMoves.push_back(pvEntry.move);
    
    game->makeMove(pvEntry.move);

    getPvLine(game, pvMoves, depth - 1);

    game->undoLastMove();
}

void printPvStatistics() {
    printf("Hits: %llu\n", hits);
    printf("Misses: %llu\n", misses);
    printf("Hit %%: %.2f\n", (float)hits/(hits + misses) * 100);
    printf("Overwrites: %llu\n", overwrites);
    printf("Collisions: %llu\n", collisions);
}