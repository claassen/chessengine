#include "search.h"
#include "pvtable.h"
#include "evaluation.h"
#include "debug.h"

#include "utils.h"

#define MOVE_IS_ALLOWED(game, turn) ((turn == WHITE && !game->currentState.whiteInCheck) || (turn == BLACK && !game->currentState.blackInCheck))
#define MOVE_SCORE_PV MOVE_SCORE_MAX

bool isThreeRepetition(Game* game) {
    int count = 0;
    
    for(auto it = game->stateHistory.begin(); it != game->stateHistory.end(); ++it) {
        if(it->hashCode == game->currentState.hashCode) {
            count++;
        }
    }

    return count > 1;
}

const int quiesce(Game* game, int alpha, int beta, volatile bool* stop) {
    if(*stop) {
        return 0;
    }

    int score = evaluate(game);

    if(score >= beta) {  
        return score;
    }

    if(score > alpha) {
        alpha = score;
    }

    pv_entry pvEntry = getPvEntry(game->currentState);
    move pvMove = pvEntry.move;

    move_list captureMoves;
    game->generateMoves(captureMoves, true);

    if(pvEntry.move != NO_MOVE && std::find(captureMoves.moves, captureMoves.moves + captureMoves.numMoves, pvMove) != captureMoves.moves + captureMoves.numMoves) {
        //There is still a small potential for hash collisions, need to check to make sure this move is actually an available move
        pvMove.score = MOVE_SCORE_PV;
        captureMoves.addMove(pvMove);
    }

    std::sort(captureMoves.moves, captureMoves.moves + captureMoves.numMoves, [](const move& a, const move& b) { return a.score > b.score; });

    int turn = game->currentState.turn;

    for(int i = 0; i < captureMoves.numMoves; ++i) {
        const move m = captureMoves.moves[i];

        game->makeMove(m);

        if((turn == WHITE && game->currentState.whiteInCheck) || (turn == BLACK && game->currentState.blackInCheck)) {
            game->undoLastMove();
            continue;
        }

        int score = -quiesce(game, -beta, -alpha, stop);

        game->undoLastMove();

        if(score > alpha) {
            alpha = score;

            if(alpha >= beta) {
                break;
            }
        }
    }

    //TODO: Deal with checks/mates?

    return alpha;
}

const int alphaBeta(Game* game, move& mv, int depth, int alpha, int beta, int ply, volatile bool* stop) {
    if(*stop) {
        return 0;
    }

    if(isThreeRepetition(game)) {
        return 0;
    }
    
    pv_entry pvEntry = getPvEntry(game->currentState);

    if(pvEntry != NO_PV_ENTRY && pvEntry.depth >= depth) {
        mv = pvEntry.move;

        if(pvEntry.scoreFlag == SCORE_EXACT) {
            return pvEntry.score;
        }
        else if(pvEntry.scoreFlag == SCORE_BETA && pvEntry.score >= beta) {
            return beta;
        }
        else if(pvEntry.scoreFlag == SCORE_ALPHA && pvEntry.score <= alpha) {
            return alpha;
        }
    }

    if(depth == 0) {
        return quiesce(game, alpha, beta, stop);
    }

    move_list moves;
    game->generateMoves(moves, false);

    move pvMove = pvEntry.move;

    if(pvEntry.move != NO_MOVE && std::find(moves.moves, moves.moves + moves.numMoves, pvMove) != moves.moves + moves.numMoves) {
        //There is still a small potential for hash collisions, need to check to make sure this move is actually an available move
        pvMove.score = MOVE_SCORE_PV;
        moves.addMove(pvMove);
    }

    std::sort(moves.moves, moves.moves + moves.numMoves, [](const move& a, const move& b) { return a.score > b.score; });

    int bestScore = -INFINITY;
    move bestMove = NO_MOVE;
    bool anyMoves = false;
    int oldAlpha = alpha;
    int turn = game->currentState.turn;    

    for(int i = 0; i < moves.numMoves; ++i) {
        const move m = moves.moves[i];

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
        int score = -alphaBeta(game, _nextMove, depth - 1, -beta, -alpha, ply + 1, stop);

        game->undoLastMove();

        if(score > bestScore) {
            bestScore = score;
            bestMove = m;
            
            if(score > alpha) {
                alpha = score;

                if(alpha >= beta) {
                    addPvMove(game->currentState, bestMove, beta, depth, SCORE_BETA);
                    mv = bestMove;
                    return alpha;
                }
            }
        }
    }   

    if(!anyMoves && (turn == WHITE ? game->currentState.whiteInCheck : game->currentState.blackInCheck)) {
        //Check mate. Current player is in check and there are no legal moves available
        return -turn * (INFINITY - ply);
    }
    else if(!anyMoves) {
        return 0;
    }

    assert(bestMove != NO_MOVE);
    
    if(alpha != oldAlpha) {
        addPvMove(game->currentState, bestMove, alpha, depth, SCORE_EXACT);
    }
    else {
        addPvMove(game->currentState, bestMove, oldAlpha, depth, SCORE_ALPHA);
    }

    mv = bestMove;
    return alpha;
}