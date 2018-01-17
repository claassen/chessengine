#include <algorithm>

#include "search.h"
#include "pvtable.h"
#include "evaluation.h"
#include "debug.h"

#include "utils.h"

#define MOVE_EXISTS(moveList, move) (move != NO_MOVE && std::find(moveList.moves, moveList.moves + moveList.numMoves, move) != moveList.moves + moveList.numMoves)
#define SORT_MOVES(moveList) (std::sort(moveList.moves, moveList.moves + moveList.numMoves, [](const move& a, const move& b) { return a.score > b.score; }))
#define MOVE_IS_ILLEGAL(game, turn) ((turn == WHITE && game->currentState.whiteInCheck) || (turn == BLACK && game->currentState.blackInCheck))
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

    if(MOVE_EXISTS(captureMoves, pvMove)) {
        //There is still a small potential for hash collisions, need to check to make sure this move is actually an available move
        pvMove.score = MOVE_SCORE_PV;
        captureMoves.addMove(pvMove);
    }

    SORT_MOVES(captureMoves);

    int turn = game->currentState.turn;

    for(int i = 0; i < captureMoves.numMoves; ++i) {
        const move m = captureMoves.moves[i];

        game->makeMove(m);

        if(MOVE_IS_ILLEGAL(game, turn)) {
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
    move pvMove = pvEntry.move;

    move_list moves;
    game->generateMoves(moves, false);

    //TODO: Pretty sure this is not necessary
    bool pvMoveIsValid = MOVE_EXISTS(moves, pvMove);

    if(pvEntry != NO_PV_ENTRY && pvEntry.depth >= depth) {
        if(pvMoveIsValid) {
            if(pvEntry.scoreFlag == SCORE_EXACT) {
                mv = pvEntry.move;
                return pvEntry.score;
            }
            else if(pvEntry.scoreFlag == SCORE_BETA && pvEntry.score >= beta) {
                mv = pvEntry.move;
                return beta;
            }
            else if(pvEntry.scoreFlag == SCORE_ALPHA && pvEntry.score <= alpha) {
                mv = pvEntry.move;
                return alpha;
            }
        }
        else {
            assert(false);
            LOG("Hash collision!");
        }
    }

    if(depth == 0) {
        return quiesce(game, alpha, beta, stop);
    }

    if(pvMoveIsValid) {
        //There is still a small potential for hash collisions, need to check to make sure this move is actually an available move
        pvMove.score = MOVE_SCORE_PV;
        moves.addMove(pvMove);
    }

    SORT_MOVES(moves);

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
        if(MOVE_IS_ILLEGAL(game, turn)) {
            game->undoLastMove();
            continue;
        }

        anyMoves = true;

        move _;
        int score = -alphaBeta(game, _, depth - 1, -beta, -alpha, ply + 1, stop);

        game->undoLastMove();

        if(score > bestScore) {
            bestScore = score;
            bestMove = m;
            
            if(score > alpha) {
                alpha = score;

                if(alpha >= beta) {
                    if(!*stop) {
                        addPvMove(game->currentState, bestMove, beta, depth, SCORE_BETA);
                    }
                    mv = bestMove;
                    return alpha;
                }
            }
        }
    }   

    if(!anyMoves && (turn == WHITE ? game->currentState.whiteInCheck : game->currentState.blackInCheck)) {
        //Check mate. Current player is in check and there are no legal moves available
        return -INFINITY + ply;
    }
    else if(!anyMoves) {
        return 0;
    }

    ASSERT(bestMove != NO_MOVE);
    
    if(!*stop) {
        if(alpha != oldAlpha) {
            addPvMove(game->currentState, bestMove, alpha, depth, SCORE_EXACT);
        }
        else {
            addPvMove(game->currentState, bestMove, oldAlpha, depth, SCORE_ALPHA);
        }
    }

    mv = bestMove;
    return alpha;
}