#ifndef ZOBRIST_H
#define ZOBRIST_H

namespace zobrist {
    extern unsigned long long pieceHashes[8][8][13];
    extern unsigned long long enPassHashes[8][8];
    extern unsigned long long castlePermHashes[16];
    extern unsigned long long turnHashes[2];

    void initialize();
}

#endif