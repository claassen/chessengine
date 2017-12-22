#include <iostream>
#include <iterator>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#include "game.h"
#include "zobrist.h"
#include "pvtable.h"
#include "search.h"
#include "utils.h"
#include "perft.h"
#include "debug.h"

#define PV_TABLE_SIZE (1024 * 1024 * 2023)  //100 MB
#define MAX_SEARCH_DEPTH 64

Game* game = new Game();
std::mutex game_state_m;

volatile bool searchDone = true;
std::mutex search_done_m;
std::condition_variable search_done_cond;

volatile bool stopSearch = false;

void search(move* bestMove) {
    std::lock_guard<std::mutex> gameStateLock(game_state_m);

    Colour turn = game->currentState.turn;

    for(int depth = 1; depth <= MAX_SEARCH_DEPTH; ++depth) {
        if(stopSearch) {
            break;
        }

        move m;
        int score = alphaBeta(game, m, depth, -INFINITY, INFINITY, turn, 1, &stopSearch);      

        if(!stopSearch) {
            std::cout << "info depth " << depth << " score cp " << ((float)score/1.0) << " pv";

            std::vector<move> pvMoves;
            getPvLine(game, pvMoves, depth);

            for(auto it = pvMoves.begin(); it != pvMoves.end(); ++it) {
                std::cout << " " << getMoveStr(*it);
            }
            std::cout << std::endl;
        
            *bestMove = m;
        }
    }

    std::lock_guard<std::mutex> searchDoneLock(search_done_m);
    searchDone = true;
    search_done_cond.notify_all();
}

void go(const std::string& input, int timeInMs) {
    //TODO: Allow taking longer when in check
    
    if(!searchDone) {
        std::unique_lock<std::mutex> searchDoneLock(search_done_m);
        while(!searchDone) {
            search_done_cond.wait(searchDoneLock);
        } 
    }

    move bestMove;
    stopSearch = false;
    searchDone = false;

    std::thread searchThread(search, &bestMove);
    searchThread.detach();

    long long start = getCurrentTimeInMs();

    while(!searchDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        long long now = getCurrentTimeInMs();

        if(now - start >= timeInMs) {
            stopSearch = true;
            break;
        }
    }

    std::cout << "bestmove " << getMoveStr(bestMove) << std::endl;
}

void position(const std::string& input) {
    std::lock_guard<std::mutex> lock(game_state_m);

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


//Why is pawn not taking bishop?
//2r2rk1/6pp/p7/1p1p3b/6P1/1RP2N1n/PKP5/4R3 w - - 0 39


//Why does Knight take bishop instead of pawn taking rook?
//r2k1b1r/p3n1pp/2ppBpq1/1RnP4/5Q2/2P3P1/PP1NP2P/R1B3K1 b - - 1 17

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cin.setf(std::ios::unitbuf);
    // signal(SIGINT, SIG_IGN);
    zobrist::initialize();
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
            //go wtime 300000 btime 300000 movestogo 40
            std::vector<std::string> parts;
            split(input, parts);

            int maxMoveTimeInMs = 60000; //10 seconds

            if(parts.size() > 1) {
                int wTimeMs = std::stoi(parts[2]);
                int bTimeMs = std::stoi(parts[4]);
                int movesToGo = 50;

                if(parts.size() > 6) {
                    movesToGo = std::stoi(parts[6]);
                }

                maxMoveTimeInMs = (game->currentState.turn == WHITE ? wTimeMs : bTimeMs) / movesToGo;
            }
            
            go(input, maxMoveTimeInMs);
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
        else if(input.compare("stats") == 0) {
            printPvStatistics();
        }
        else if(input.substr(0, 4).compare("move") == 0) {
            std::string move = input.substr(5);
            game->makeMove(move);
        }
        else if(input.compare("score") == 0) {
            std::cout << "Score black: " << game->getScore(BLACK) << std::endl;
            std::cout << "Score white: " << game->getScore(WHITE) << std::endl;
        }
    }
}