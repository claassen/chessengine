#include "evaluation.h"

#define PIECE_AT(row, col) (game->currentState.board[row + 2][col + 2])

//Pawns should move toward opposite end, also encourage the 2 center pawns to move out
const int pawnPositionScores[8][8] = {
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {  20,  20,  20,  20,  20,  20,  20,  20 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0, -10, -10,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 }
};

//Knights should stay away from edges and move towards center
const int knightPositionScores[8][8] = {
    { -10, -10, -10, -10, -10, -10, -10, -10 },
    { -10,   5,   5,   5,   5,   5,   5, -10 },
    { -10,   5,  10,  10,  10,  10,   5, -10 },
    { -10,   5,  10,  20,  20,  10,   5, -10 },
    { -10,   5,  10,  20,  20,  10,   5, -10 },
    { -10,   5,  10,  10,  10,  10,   5, -10 },
    { -10,   5,   5,   5,   5,   5,   5, -10 },
    { -10, -10, -10, -10, -10, -10, -10, -10 }
};

//Bishops should move towards center
const int bishopPositionScores[8][8] = {
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   5,   5,   5,   5,   5,   5,   0 },
    {   0,   5,  10,  10,  10,  10,   5,   0 },
    {   0,   5,  10,  20,  20,  10,   5,   0 },
    {   0,   5,  10,  20,  20,  10,   5,   0 },
    {   0,   5,  10,  10,  10,  10,   5,   0 },
    {   0,   5,   5,   5,   5,   5,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 }
};

//Rooks - not really sure on this one
const int rookPositionScores[8][8] = {
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 }
};

//Queen should move towards center
const int queenPositionScores[8][8] = {
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   5,   5,   5,   5,   5,   5,   0 },
    {   0,   5,  10,  10,  10,  10,   5,   0 },
    {   0,   5,  10,  20,  20,  10,   5,   0 },
    {   0,   5,  10,  20,  20,  10,   5,   0 },
    {   0,   5,  10,  10,  10,  10,   5,   0 },
    {   0,   5,   5,   5,   5,   5,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0 }
};

//TODO: Need different scores for end game
//King should stay back and try to castle and avoid corners
const int kingPositionScores[8][8] = {
    {-100,  -5,  -5,  -5,  -5,  -5,  -5,-100 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5 },
    {-100,   0,  10,   0,   0,   0,  10,-100 }
};

int evaluate(Game* game) {
    int score = 0;

    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            Piece p = PIECE_AT(i, j);

            switch(p) {
                //Black pieces
                case bP: 
                    score += 100;    
                    score += pawnPositionScores[7-i][j];
                    break;
                case bN: 
                    score += 320; 
                    score += knightPositionScores[7-i][j];
                    break;
                case bB: 
                    score += 330; 
                    score += bishopPositionScores[7-i][j];
                    break;
                case bR: 
                    score += 500; 
                    score += rookPositionScores[7-i][j];
                    break;
                case bQ: 
                    score += 900; 
                    score += queenPositionScores[7-i][j];
                    break;
                case bK: 
                    score += 100000; 
                    score += kingPositionScores[7-i][j];
                    break;

                //White pieces
                case wP: 
                    score -= 100;
                    score -= pawnPositionScores[i][j];
                    break;
                case wN: 
                    score -= 320;
                    score -= knightPositionScores[i][j];
                    break;
                case wB: 
                    score -= 330; 
                    score -= bishopPositionScores[i][j];
                    break;
                case wR: 
                    score -= 500;
                    score -= rookPositionScores[i][j];
                    break;
                case wQ: 
                    score -= 900; 
                    score -= queenPositionScores[i][j];
                    break;
                case wK: 
                    score -= 100000;
                    score -= kingPositionScores[i][j];
                    break;
                default: break;
            }
        }
    }

    //Mobility score?

    //Protect king

    //Sentries

    //Pawn structure (isolated, blocked)

    return -game->currentState.turn * score;
}