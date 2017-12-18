#include "search.h"
#include "pvtable.h"
#include "debug.h"

#define MOVE_IS_ALLOWED(game, turn) ((turn == WHITE && !game->currentState.whiteInCheck) || (turn == BLACK && !game->currentState.blackInCheck))
#define MOVE_SCORE_PV MOVE_SCORE_MAX

const int alphaBeta(Game* game, move& mv, int depth, int alpha, int beta, Colour turn, int ply, volatile bool* stop) {
    if(*stop) {
        return 0;
    }

    std::vector<move> moves;
    game->generateMoves(turn, moves);
    
    if(depth == 0) {
        std::vector<move> opponentMoves;

        //TODO: This breaks things!
        // game->generateMoves((Colour)-turn, opponentMoves);

        return -turn * game->getScore(turn, moves, opponentMoves);
    }

    move pvMove = getPvMove(game->currentState);

    if(pvMove != NO_MOVE && std::find(moves.begin(), moves.end(), pvMove) != moves.end()) {
        //There is still a small potential for hash collisions, need to check to make sure this move is actually an available move
        pvMove.score = MOVE_SCORE_PV;
        moves.insert(moves.begin(), pvMove);
    }

    std::sort(moves.begin(), moves.end(), [](const move& a, const move& b) { return a.score > b.score; });

    int maxScore = INT_MIN;
    move bestMove;
    bool anyMoves = false;

    for(auto it = moves.begin(); it != moves.end(); ++it) {
        move m = *it;
        game->makeMove(m);

        //We have to actually make a move to see if it is legal in terms of not leaving the player in check
        //which we defer to checking here as moves may be skipped by alpha-beta pruning in which case we never
        //actually need to check this
        if((turn == WHITE && game->currentState.whiteInCheck) || (turn == BLACK && game->currentState.blackInCheck)) {
            game->undoLastMove();
            continue;
        }

        anyMoves = true;

        move _nextMove;
        int score = -alphaBeta(game, _nextMove, depth - 1, -beta, -alpha, (Colour)-turn, ply + 1, stop);

        if(score > maxScore) {
            maxScore = score;
            bestMove = m;
        }

        if(score > alpha) {
            alpha = score;
        }

        game->undoLastMove();

        if(alpha >= beta) {
            break;
        }
    }

    if(!anyMoves && (turn == WHITE ? game->currentState.whiteInCheck : game->currentState.blackInCheck)) {
        //Check mate. Current player is in check and there are no legal moves available
        return -turn * (INFINITY - ply);
    }

    addPvMove(game->currentState, bestMove);

    mv = bestMove;
    return maxScore;
}