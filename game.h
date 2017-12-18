#ifndef GAME_H
#define GAME_H

#include <string>
#include <stack>
#include <vector>
#include <climits>

#include "debug.h"

#define STARTPOS "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

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
    Piece board[8][8];
    Colour turn = WHITE;
    int castlePerm;
    enPassLocation enPass = NO_EN_PASS;
    int fiftyMove;
    int turns;
    bool whiteInCheck;
    bool blackInCheck;
    size_t hashCode;
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

class Game {
public:
    gameState currentState;
    std::stack<gameState, std::vector<gameState>> stateHistory;
    void startPosition(const std::string& fen);
    void makeMove(const std::string& move);
    void makeMove(const move& move);
    void undoLastMove();
    const bool isAttacked(unsigned int x, unsigned int y, const Colour attackingColour);
    void addQuietMove(const Colour turn, std::vector<move>& moves, move move);
    void addCaptureMove(const Colour turn, std::vector<move>& moves, move move, const Piece pieceMoved, const Piece capturedPiece);
    void generateMoves(const Colour turn, std::vector<move>& moves);
    void generatePawnMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    void generateKnightMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    void generateBishopMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    void generateRookMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    void generateQueenMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    void generateKingMoves(const Colour turn, std::vector<move>& moves, unsigned int x, unsigned int y);
    int getScore(int turn, const std::vector<move>& availableMyMoves, const std::vector<move>& availableOpponentMoves);
    void print();
};

#endif