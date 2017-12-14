#include <iostream>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

// #define NDEBUG

#include "testengine.h"
#include "game.h"
#include "pvtable.h"
#include "search.h"
#include "utils.h"
#include "logging.h"

#define PV_TABLE_SIZE (1024 * 1024 * 100)  //100 MB
#define MAX_SEARCH_DEPTH 8


//Mate in 3ish (fixed):
//position fen r1b3n1/pp5r/n2k4/2p2pp1/q7/8/8/b3K3 w - f6 0 28 moves e1f1

//Mate is 1 (fixed):
//position fen r1b1kbn1/pppp1pp1/6p1/P1n5/5q2/8/7r/3K4 w - - 1 21 moves a5a6

//Stupid queen move (fixed):
//position fen rnb1kBnr/ppp2ppp/8/8/P1pp1P2/6P1/P2P1K1P/q4B2 b kq - 1 12

//Stupid queen move (fixed):
//position fen rnb1kbnr/pppp1ppp/8/4p3/4P2q/2N3P1/PPPP1P1P/R1BQKBNR b KQkq - 0 3


Game* game = new Game();
std::mutex search_m;

void search(move* bestMove, volatile bool* stop) {
    std::lock_guard<std::mutex> lock(search_m);

    for(int depth = 1; depth <= MAX_SEARCH_DEPTH; ++depth) {
        if(*stop) {
            return;
        }

        move move;
        int score = alphaBeta(game, move, depth, INT_MIN, INT_MAX, BLACK, 1, stop);      

        if(!*stop) {
            LOG(std::string("") + "info depth " + std::to_string(depth) + " score cp " + std::to_string(score) + " move " + getMoveStr(move));
            std::cout << "info depth " << depth << " score cp " << score << " move " << getMoveStr(move) << std::endl;
        
            *bestMove = move;
        }
    }
}

void position(const std::string& input) {
    std::lock_guard<std::mutex> lock(search_m);

    std::vector<std::string> moves;

    if(input.substr(9, 8).compare("startpos") == 0) {
        split(input.substr(23), moves);
        game->startPosition(STARTPOS);
    }
    else {
        int movesStart = input.find("moves");
        if(movesStart != -1) {
            split(input.substr(movesStart + 6), moves);              
        }
        game->startPosition(input.substr(13, movesStart - 1));
    }

    for(auto it = moves.begin(); it != moves.end(); ++it) {
        game->makeMove(*it);
    }
}

void go(const std::string& input) {
    move bestMove;
    volatile bool stop = false;

    // std::thread searchThread(search, &bestMove, &stop);
    // searchThread.detach();
    search(&bestMove, &stop);

    //TODO: Allow this to finish earlier if we reach max-depth
    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    stop = true;

    std::cout << "bestmove " << getMoveStr(bestMove) << std::endl;
}

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cin.setf(std::ios::unitbuf);
    // signal(SIGINT, SIG_IGN);

    initPvTable(PV_TABLE_SIZE);

    std::string input;    
    
    while(true) {
        std::getline(std::cin, input);

        if(input.size() != 0) {
            LOG_INPUT(input);
        }

        if(input.compare("uci") == 0) {
            std::cout << "id name TestEngine" << std::endl;
            std::cout << "id author Mike" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if(input.compare("isready") == 0) {
            std::cout << "readyok" << std::endl;
        }
        else if(input.compare("ucinewgame") == 0) {
            game = new Game();
        }
        else if(input.substr(0, 8).compare("position") == 0) {
            position(input);
        }
        else if(input.substr(0, 2).compare("go") == 0) {
            std::vector<std::string> parts;
            split(input, parts);

            

            //TODO: Parse wtime
            //TODO: Deal with playing either colour
            go(input);
        }
        else if(input.compare("stop") == 0) {
            // stop = true;
        }
    }
}