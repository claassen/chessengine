#include <iostream>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#include "game.h"
#include "pvtable.h"
#include "search.h"
#include "utils.h"
#include "perft.h"
#include "debug.h"

#define PV_TABLE_SIZE (1024 * 1024 * 100)  //100 MB
#define MAX_SEARCH_DEPTH 8

Game* game = new Game();
std::mutex search_m;

void search(move* bestMove, volatile bool* stop) {
    std::lock_guard<std::mutex> lock(search_m);

    Colour turn = game->currentState.turn;

    for(int depth = 1; depth <= MAX_SEARCH_DEPTH; ++depth) {
        if(*stop) {
            return;
        }

        move move;
        int score = alphaBeta(game, move, depth, -INFINITY, INFINITY, turn, 1, stop);      

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

    int movesStart = input.find("moves");
    if(movesStart != -1) {
        split(input.substr(movesStart + 6), moves);              
    }

    if(input.substr(9, 8).compare("startpos") == 0) {
        //position startpos
        game->startPosition(STARTPOS);
    }
    else {
        //position fen <fen>
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
    INIT_LOGGING();

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
            //position startpos [moves e2e4...]
            //position fen <fen> [moves e2e4...]
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
        else if(input.compare("perft suite") == 0) {
            perftTestSuite(game);
            return 0;
        }
        else if(input.substr(0, 5).compare("perft") == 0) {
            //perft <depth>
            perftDivide(game, std::stoi(input.substr(6)));
        }
        else if(input.compare("p") == 0) {
            game->print();
        }
    }
}