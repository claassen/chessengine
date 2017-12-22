#include "zobrist.h"

#include <random>
#include <cmath>

namespace zobrist {
    std::random_device rd;
    std::mt19937_64 e2(rd());
    std::uniform_int_distribution<unsigned long long> dist(std::pow(2,61), std::llround(std::pow(2,62)));

    unsigned long long pieceHashes[8][8][13];
    unsigned long long enPassHashes[8][8];
    unsigned long long castlePermHashes[16];
    unsigned long long turnHashes[2];

    void initialize() {
        for(int row = 0; row < 8; row++) {
            for(int col = 0; col < 8; col++) {
                for(int p = 0; p < 13; p++) {
                    pieceHashes[row][col][p] = dist(e2);
                }
            }
        }

        for(int row = 0; row < 8; row++) {
            for(int col = 0; col < 8; col++) {
                enPassHashes[row][col] = dist(e2);
            }
        }

        for(int i = 0; i < 16; i++) {
            castlePermHashes[i] = dist(e2);
        }

        turnHashes[0] = dist(e2);
        turnHashes[1] = dist(e2);
    }
}