#include <vector>
#include <iostream>
#include <fstream>

#include "perft.h"
#include "utils.h"
#include "debug.h"

static long leafNodes = 0;

void perft(Game* game, int depth, move& moveMade) {
    if(depth == 0) {
        leafNodes++;
        return;
    }

    std::vector<move> moves;
    game->generateMoves(game->currentState.turn, moves);

    for(auto it = moves.begin(); it != moves.end(); ++it) {
        game->makeMove(*it);

        Colour turnBeforeMove = (Colour)-game->currentState.turn;

        if((turnBeforeMove == WHITE && game->currentState.whiteInCheck) || (turnBeforeMove == BLACK && game->currentState.blackInCheck)) {
            //Illegal move
            game->undoLastMove();       
            continue;
        }

        perft(game, depth - 1, *it);

        game->undoLastMove();
    } 
}

void perftTestSuite(Game* game) {
    std::ifstream infile("perftsuite.epd");
    std::string line;

    move noMove;

    while(std::getline(infile, line)) {
        if(line.substr(0, 2).compare("//") == 0) {
            continue;
        }

        std::vector<std::string> parts;
        split(line, parts);

        int depthToCheck = 6;

        if(parts.size() < 18) {
            depthToCheck = 5;
        }

        int expectedNodeCounts[6] = {
            std::stoi(parts[7]),
            std::stoi(parts[9]),
            std::stoi(parts[11]),
            std::stoi(parts[13]),
            std::stoi(parts[15]),
            (depthToCheck == 6 ? std::stoi(parts[17]) : 0)
        };

        std::string fen = line.substr(0, line.find(";") - 1);

        std::cout << "Testing FEN: " << fen << std::endl;

        for(int i = 1; i <= depthToCheck; i++) {
            leafNodes = 0;

            game->startPosition(fen);

            perft(game, i, noMove);

            printf("Depth: %d, nodes: %ld\n", i, leafNodes);

            if(leafNodes != expectedNodeCounts[i - 1]) {
                std::cout << "MISMATCH!!! Expected: " << expectedNodeCounts[i - 1] << " actual: " << leafNodes << std::endl;
                break;
            }
        }

        std::cout << std::endl;
    }
}

void perftDivide(Game* game, int depth) {
    std::cout << std::endl;

    std::vector<move> moves;
    game->generateMoves(game->currentState.turn, moves);

    int totalNodes = 0;

    for(auto it = moves.begin(); it != moves.end(); ++it) {
        leafNodes = 0;

        game->makeMove(*it);

        Colour turnBeforeMove = (Colour)-game->currentState.turn;

        if((turnBeforeMove == WHITE && game->currentState.whiteInCheck) || (turnBeforeMove == BLACK && game->currentState.blackInCheck)) {
            //Illegal move
            game->undoLastMove();       
            continue;
        }

        perft(game, depth - 1, *it);

        game->undoLastMove();

        std::cout << getMoveStr(*it) << ": " << leafNodes << std::endl;

        totalNodes += leafNodes;
    }

    std::cout << std::endl << "Total nodes: " << totalNodes << std::endl;
}