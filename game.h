#ifndef GAME_H
#define GAME_H

#include <string>
#include <stack>
#include <vector>

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

struct location {
    int x;
    int y;
};

struct gameState {
    Piece board[8][8];
    Colour turn = WHITE;
    int castlePerm;
    location enPass = {
        .x = -1,
        .y = -1
    };
    int fiftyMove;
    int turns;
    bool whiteInCheck;
    bool blackInCheck;
};

struct move {
    int fromX;
    int fromY;
    int toX;
    int toY;
    PieceType promotion;
    int score;
};

class Game {
public:
    gameState currentState;
    std::stack<gameState, std::vector<gameState>> stateHistory;
    void startPosition(const std::string& fen);
    void makeMove(const std::string& move);
    void makeMove(const move& move);
    void undoLastMove();
    const bool isAttacked(int x, int y, const Colour attackingColour);
    void addQuietMove(const Colour turn, std::vector<move>& moves, move move);
    void addCaptureMove(const Colour turn, std::vector<move>& moves, move move, const Piece pieceMoved, const Piece capturedPiece);
    void generateMoves(const Colour turn, std::vector<move>& moves);
    void generatePawnMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    void generateKnightMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    void generateBishopMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    void generateRookMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    void generateQueenMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    void generateKingMoves(const Colour turn, std::vector<move>& moves, int x, int y);
    int getScore(int turn, const std::vector<move>& availableMyMoves, const std::vector<move>& availableOpponentMoves);
    const int negamax(move& mv, const std::vector<move>& moves, int depth, int alpha, int beta, Colour turn);
    void print();
};

#endif