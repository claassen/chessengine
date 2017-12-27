#ifndef GAME_H
#define GAME_H

#include <string>
#include <stack>
#include <vector>
#include <climits>

#include "debug.h"

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define INFINITY (INT_MAX - 1)

enum Colour {
    WHITE = 1,
    BLACK = -1,
    NONE = 0
};

enum PieceType {
    Empty = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    OffBoard
};

enum Piece {
    empty = 0,
    bP,
    bN,
    bB,
    bR,
    bQ,
    bK,
    wP,
    wN,
    wB,
    wR,
    wQ,
    wK,
    off_board
};

enum CastlePerm {
    K = 1,
    Q = 2,
    k = 4,
    q = 8
};

struct enPassLocation {
    unsigned int x : 3;
    unsigned int y : 3;

    void set(unsigned int i) {
        this->x = (0x38 & i) >> 3;
        this->y = ( 0x7 & i);
    }

    enPassLocation(unsigned int i) {
        this->set(i);
    }

    enPassLocation operator=(unsigned int i) {
        this->set(i);
        return *this;
    }

    operator unsigned int() {
        return (x << 3) + y;
    }
};

#define NO_EN_PASS 0
#define EN_PASS(x, y) (((0x7 & x) << 3) + (0x7 & y))

struct gameState {
    Piece board[12][12];
    Colour turn = WHITE;
    unsigned int castlePerm : 4;
    enPassLocation enPass = NO_EN_PASS;
    int fiftyMove : 5;
    int turns : 8;
    bool whiteInCheck;
    bool blackInCheck;
    unsigned long long hashCode;
};

#define MOVE_SCORE_MAX  16382 //15 bits signed
#define MOVE_SCORE_MIN -16383 //15 bits signed

struct move {
    unsigned int fromX  : 3;
    unsigned int fromY  : 3;
    unsigned int toX    : 3;
    unsigned int toY    : 3;
    PieceType promotion : 4;
    bool isCastle       : 1;
    int score           : 15;

    bool operator==(const move& rhs) {
        return fromX == rhs.fromX &&
            fromY == rhs.fromY &&
            toX == rhs.toX &&
            toY == rhs.toY &&
            promotion == rhs.promotion;
    }

    bool operator!=(const move& rhs) {
        return !(*this == rhs);
    }
};

#define NO_MOVE (move{ .fromX = 0, .fromY = 0, .toX = 0, .toY = 0, .promotion = Empty })

struct move_list {
    //From what I can find, 208 is the maximum number of moves at any position in chess
    move moves[256];
    int numMoves = 0;

    void addMove(const move mv) {
        moves[numMoves++] = mv;
    }
};

class Game {
public:
    gameState currentState;
    std::vector<gameState> stateHistory;
    void startPosition(const std::string& fen);
    void makeMove(const std::string& move);
    void makeMove(const move& move);
    void undoLastMove();
    const bool isAttacked(unsigned int x, unsigned int y, const Colour attackingColour);
    void addQuietMove(move_list& moveList, move move);
    void addCaptureMove(move_list& moveList, move move, const Piece pieceMoved, const Piece capturedPiece);
    void generateMoves(move_list& moveList, bool capturesOnly);
    void generatePawnMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    void generateKnightMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    void generateBishopMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    void generateRookMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    void generateQueenMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    void generateKingMoves(move_list& moveList, unsigned int x, unsigned int y, bool capturesOnly);
    int getScore(int turn);
    void print();
};

#endif