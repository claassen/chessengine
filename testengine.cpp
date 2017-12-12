#include <iostream>
#include <iterator>
#include <algorithm>

#define NDEBUG

#include "testengine.h"
#include "game.h"
#include "utils.h"
#include "logging.h"

//Performance test:
//rq1k1b1r/3ppBpp/1pnp1n2/p5N1/P1Q2P2/1PN1B1P1/2P4P/R3K2b b Q - 4 17

//segfault!
//position startpos moves e2e4 b8a6 d2d4 a8b8 c2c4 b8a8 f2f4 c7c5 d4c5 a6c5 b2b4 c5e4 a2a4 a8b8 a4a5 d8a5 c4c5 a5a1 c5c6

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cin.setf(std::ios::unitbuf);
    // signal(SIGINT, SIG_IGN);

    //I don't know why, but for some specific combination of moves there is a segfault when 
    //creating the Game on the stack and creating it on the heap and never freeing it fixes it
    Game* game = new Game();

    std::string input;    
    
    while(true) {
        std::getline(std::cin, input);
        LOG_INPUT(input);

        if(input.compare("uci") == 0) {
            std::cout << "id name TestEngine" << std::endl;
            std::cout << "id author Mike" << std::endl;
            std::cout << "uciok" << std::endl;
        }
        else if(input.compare("isready") == 0) {
            std::cout << "readyok" << std::endl;
        }
        else if(input.compare("ucinewgame") == 0) {
            //Start new game
            game = new Game();
        }
        else if(input.substr(0, 8).compare("position") == 0) {
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

            std::vector<move> myMoves;
            game->generateMoves(BLACK, myMoves);
            
            std::sort(myMoves.begin(), myMoves.end(), [](move a, move b) { return a.score > b.score; });

            move m;
            int score = game->negamax(m, myMoves, 5, INT_MIN, INT_MAX, BLACK);

            std::cout << "bestmove " << getMoveStr(m.fromX, m.fromY, m.toX, m.toY, m.promotion) << std::endl;
        }
        else if(input.compare("go") == 0) {
            //Start thinking
            LOG("Go command recieved");
        }
        else if(input.compare("stop") == 0) {
            // std::cout << "bestmove e7e5" << std::endl;
        }
    }
}