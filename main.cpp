#include <iostream>
#include <iterator>
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
#include "tcpsocket.h"

#define PV_TABLE_SIZE (1024 * 1024 * 2023)
#define MAX_SEARCH_DEPTH 64

Game* game = new Game();
std::mutex game_state_m;

volatile bool searchDone = true;
std::mutex search_done_m;
std::condition_variable search_done_cond;

volatile bool stopSearch = false;
volatile bool stopPonder = false;

int movesToGo = 50; //default number of moves estimated for a game

void search(move* bestMove) {
    std::lock_guard<std::mutex> gameStateLock(game_state_m);

    for(int depth = 1; depth <= MAX_SEARCH_DEPTH; ++depth) {
        if(stopSearch) {
            break;
        }

        move m;
        int score = alphaBeta(game, m, depth, -INFINITY, INFINITY, 1, &stopSearch);      

        if(!stopSearch) {
            std::cout << "info depth " << depth << " score cp " << ((float)score/1.0) << " pv";

            LOG(std::string("info depth ") + std::to_string(depth) + " score cp " + std::to_string(((float)score/1.0)) + " pv");

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

void ponder() {
    std::lock_guard<std::mutex> gameStateLock(game_state_m);

    for(int depth = 1; depth <= MAX_SEARCH_DEPTH; ++depth) {
        if(stopPonder) {
            break;
        }

        move m;
        alphaBeta(game, m, depth, -INFINITY, INFINITY, 1, &stopPonder);
    }
}

void go(int timeInMs, move& moveMade) {
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

    long long start = getCurrentTimeInMs();

    while(!searchDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        long long now = getCurrentTimeInMs();

        if(now - start >= timeInMs) {
            stopSearch = true;
            break;
        }
    }

    LOG(getMoveStr(bestMove));
    
    moveMade = bestMove;

    searchThread.join();

    game->makeMove(bestMove);

    stopPonder = false;

    std::thread ponderThread(ponder);
    ponderThread.detach();
}

void position(const std::string& input) {
    stopPonder = true;
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
        game->makeMove(getMove(*it));
    }
}

void uci() {
    std::cout << "id name TestEngine" << std::endl;
    std::cout << "id author Michael Claassen" << std::endl;
    std::cout << "uciok" << std::endl;

    std::string input;

    while(true) {
        std::getline(std::cin, input);

        if(input.size() != 0) {
            LOG_INPUT(input);
        }

        if(input.compare("isready") == 0) {
            std::cout << "readyok" << std::endl;
        }
        else if(input.compare("ucinewgame") == 0) {
            stopPonder = true;
            {
                std::lock_guard<std::mutex> lock(game_state_m);
            }
            delete game;
            game = new Game();
            movesToGo = 50;
        }
        else if(input.substr(0, 8).compare("position") == 0) {
            //position startpos [moves e2e4...]
            //position fen <fen> [moves e2e4...]
            position(input);
        }
        else if(input.substr(0, 2).compare("go") == 0) {
            //go wtime 300000 btime 300000 [movestogo 50]
            std::vector<std::string> parts;
            split(input, parts);

            int maxMoveTimeInMs = 7000; //10 seconds

            if(parts.size() > 1) {
                int wTimeMs = std::stoi(parts[2]);
                int bTimeMs = std::stoi(parts[4]);

                if(parts.size() > 6) {
                    movesToGo = std::stoi(parts[6]);
                }
                else {
                    movesToGo--;            
                }

                maxMoveTimeInMs = std::max(1000, (game->currentState.turn == WHITE ? wTimeMs : bTimeMs) / (std::max(1, movesToGo)));
            }
            
            LOG(std::string("Moves to go: ") + std::to_string(movesToGo));
            LOG(std::string("Move time (ms): ") + std::to_string(maxMoveTimeInMs));

            move bestMove;
            go(maxMoveTimeInMs, bestMove);
            std::cout << "bestmove " << getMoveStr(bestMove) << std::endl;
        }
        else if(input.compare("stop") == 0) {
            stopSearch = true;
        }

        //TODO: quit
    }
}

void tb_position(const std::string& fen) {
    stopPonder = true;
    std::lock_guard<std::mutex> lock(game_state_m);
    std::cout << "fen: " << fen << std::endl;
    game->startPosition(fen);
    game->print();
}

void tb() {
    TCPSocket server("54.173.172.97", 1234);
    // TCPSocket server("0.0.0.0", 1234);

    std::cout << "Enter tournament name: ";
    std::string tourneyName;
    std::cin >> tourneyName;

    std::cout << "\n\nEnter player name: ";
    std::string playerName;
    std::cin >> playerName;

    server.writeLine(std::string("JOIN ") + tourneyName + " " + playerName + "\n");

    std::string gameId;
    int timeLimit;
    int increment;

    while(true) {
        std::string input = server.readLine();

        if(input.size() != 0) {
            std::cout << input << std::endl;
        }

        if(input.substr(0, 11).compare("GAME_PAIRED") == 0) {
            std::vector<std::string> parts;
            split(input, parts);

            gameId = parts[1];

            timeLimit = std::stoi(parts[4]);
            increment = std::stoi(parts[5]);

            server.writeLine(std::string("ACK ") + gameId + "\n");
        }
        else if(input.substr(0, 12).compare("GAME_STARTED") == 0) {
            stopPonder = true;
            {
                std::lock_guard<std::mutex> lock(game_state_m);
            }
            delete game;
            game = new Game();
            movesToGo = 50;
        }
        else if(input.substr(0, 9).compare("YOUR_MOVE") == 0) {
            std::vector<std::string> parts;
            split(input, parts);

            float wTimeS = std::stof(parts[4]);
            float bTimeS = std::stof(parts[5]);

            std::string fen = parts[6];
            for(int i = 7; i < parts.size(); i++) {
                fen = fen + " " + parts[i];
            }

            tb_position(fen);

            int maxMoveTimeInMs = ((game->currentState.turn == WHITE ? wTimeS : bTimeS) / movesToGo--) * 1000;

            std::cout << "Moves to go: " << std::to_string(movesToGo) << std::endl;
            std::cout << "Move time (ms): " << std::to_string(maxMoveTimeInMs) << std::endl;

            move bestMove;
            go(maxMoveTimeInMs, bestMove);
            server.writeLine(std::string("MOVE ") + gameId + " " + getMoveStr(bestMove) + "\n");
        }
    }
}

int main(int argc, char *argv[]) {
    std::cout.setf(std::ios::unitbuf);
    std::cin.setf(std::ios::unitbuf);
    // signal(SIGINT, SIG_IGN);
    zobrist::initialize();
    initPvTable(PV_TABLE_SIZE);

    INIT_LOGGING();

    std::string input;

    while(true) {
        std::getline(std::cin, input);

        if(input.compare("uci") == 0) {
            uci();
        }
        else if(input.compare("tb") == 0) {
            tb();
        }
        else if(input.compare("perft suite") == 0) {
            perftTestSuite(game);
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
            std::string moveStr = input.substr(5);
            game->makeMove(getMove(moveStr));
        }
    }
}