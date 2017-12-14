#include <cstdlib>
#include <assert.h>
#include <sstream>
#include <algorithm>

#include "utils.h"
#include "game.h"

#define PIECE_AT(y, x)        ((x < 0 || x > 7 || y < 0 || y > 7) ? off_board : currentState.board[y][x])
#define MOVE_IS_ALLOWED(turn) ((turn == WHITE && !currentState.whiteInCheck) || (turn == BLACK && !currentState.blackInCheck))

//Copied from boost
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> h;
    seed ^= h(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

const PieceType pieceTypes[14] = {
    Empty,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    OffBoard
};

const Colour pieceColours[14] = {
    NONE,
    BLACK,
    BLACK,
    BLACK,
    BLACK,
    BLACK,
    BLACK,
    WHITE,
    WHITE,
    WHITE,
    WHITE,
    WHITE,
    WHITE,
    NONE
};

const PieceType promotionTypes[4] = {
    Knight,
    Bishop,
    Rook,
    Queen
};

const int MvvLVA [7][7] = {
//attacker:   p   n   b   r   q   k    victim:
        { 0,  0,  0,  0,  0,  0,  0 },
        { 0,  6,  5,  4,  3,  2,  1 }, //pawn
        { 0, 13, 12, 11, 10,  9,  8 }, //knight
        { 0, 20, 19, 18, 17, 16, 15 }, //bishop
        { 0, 27, 26, 25, 24, 23, 22 }, //rook
        { 0, 34, 33, 32, 31, 30, 29 }, //queen
        { 0, 99, 99, 99, 99, 99, 99 }, //king (taking king is just as good with any piece) TODO: Not even consider king captures legal moves?
};

void Game::startPosition(const std::string& fen) {
    stateHistory = std::stack<gameState, std::vector<gameState>>();

    currentState.castlePerm = 0;
    currentState.enPass = {
        .x = -1,
        .y = -1
    };

    std::vector<std::string> parts;
    split(fen, parts);

    //Position
    std::string position = parts[0];
    int x = 0;
    int y = 0;
    for(auto it = position.begin(); it < position.end(); ++it) {
        char c = *it;

        if(c == '/') {
            ++y;
            x = 0;
        }
        else if(c >= '0' && c <= '9') {
            for(int i = 0; i < c - '0'; i++) {
                currentState.board[y][x] = empty;
                ++x;
            }
        }
        else if(c != ' ') {
            currentState.board[y][x] = getPiece(c);
            ++x;
        }
    }

    //Turn
    std::string turn = parts[1];
    currentState.turn = getTurn(turn.at(0));

    //Castle permissions
    std::string castlePerms = parts[2];
    if(castlePerms.at(0) != '-') {
        for(auto it = castlePerms.begin(); it < castlePerms.end(); ++it) {        
            currentState.castlePerm |= getCastlePerm(*it);
        }
    }
    
    //En passant
    std::string enPass = parts[3];
    if(enPass.at(0) != '-') {
        currentState.enPass = getLocation(enPass.at(0), enPass.at(1));
    }

    //Fifty move
    currentState.fiftyMove = std::stoi(parts[4]);

    //Turns
    currentState.turns = std::stoi(parts[5]);

    //Set hash code
    currentState.hashCode = 0;
    hash_combine(currentState.hashCode, fen);
}

void Game::makeMove(const std::string& move) {
    location fromLoc = getLocation(move.at(0), move.at(1));
    location toLoc = getLocation(move.at(2), move.at(3));

    PieceType promotion = Empty;

    if(move.length() > 4) {
        promotion = getPiecePromotionType(move.at(4));
    }

    makeMove({  
        .fromX = fromLoc.x,
        .fromY = fromLoc.y,
        .toX = toLoc.x,
        .toY = toLoc.y,
        .promotion = promotion
    });
}

void Game::makeMove(const move& m) {
    stateHistory.push(currentState);

    const Piece p = currentState.board[m.fromY][m.fromX];

    if(p == bP || p == wP || currentState.board[m.toY][m.toX] != empty || (m.toX == currentState.enPass.x && m.toY == currentState.enPass.y)) {
        //Pawn move or capture, reset 50 move rule
        currentState.fiftyMove = 0;
    }
    else {
        ++currentState.fiftyMove;
    }

    if(m.promotion != Empty) {
        currentState.board[m.toY][m.toX] = getPiece(m.promotion, currentState.turn);   
    }
    else {
        currentState.board[m.toY][m.toX] = p;
    }

    //En passant capture
    if(m.toX == currentState.enPass.x && m.toY == currentState.enPass.y) {
        if(currentState.turn == WHITE) {
            currentState.board[currentState.enPass.y + 1][currentState.enPass.x] = empty;
        }
        else {
            currentState.board[currentState.enPass.y - 1][currentState.enPass.x] = empty;
        }

        currentState.enPass = {
            .x = -1,
            .y = -1
        };
    }

    currentState.board[m.fromY][m.fromX] = empty;

    //Castling
    if(p == bK && m.toX == m.fromX - 2) {
        //Black king side
        currentState.castlePerm &= ~q;
        currentState.board[0][m.toX + 1] = bR;
        currentState.board[0][0] = empty;
    }
    else if(p == bK && m.toX == m.fromX + 2) {
        //Black queen side
        currentState.castlePerm &= ~k;
        currentState.board[0][m.toX - 1] = bR;
        currentState.board[0][7] = empty;
    }
    else if(p == wK && m.toX == m.fromX - 2) {
        //White king side
        currentState.castlePerm &= ~Q;
        currentState.board[7][m.toX + 1] = wR;
        currentState.board[7][0] = empty;
    }
    else if(p == wK && m.toX == m.fromX + 2) {
        //White queen side
        currentState.castlePerm &= ~K;
        currentState.board[7][m.toX - 1] = wR;
        currentState.board[7][7] = empty;
    }

    //Reset en pasant
    currentState.enPass = {
        .x = -1,
        .y = -1
    };

    //En passant
    if((p == bP || p == wP) && std::abs(m.fromY - m.toY) > 1) {
        currentState.enPass = {
            .x = m.fromX,
            .y = m.fromY < m.toY ? m.fromY + 1 : m.fromY - 1
        };
    }

    //Update castling permissions
    if(p == wK) {
        //Moved king, both permissions lost
        currentState.castlePerm &= ~Q;
        currentState.castlePerm &= ~K;
    }
    else if(p == bK) {
        //Moved king, both permissions lost
        currentState.castlePerm &= ~q;
        currentState.castlePerm &= ~k;
    }
    else if(p == wR && m.fromX == 0 && m.fromY == 7) {
        //White queen side
        currentState.castlePerm &= ~Q;
    }
    else if(p == wR && m.fromX == 7 && m.fromY == 7) {
        //White king side
        currentState.castlePerm &= ~K;
    }
    else if(p == bR && m.fromX == 0 && m.fromY == 0) {
        //Black queen side
        currentState.castlePerm &= ~q;
    }
    else if(p == bR && m.fromX == 7 && m.fromY == 0) {
        //Black king side
        currentState.castlePerm &= ~k;
    }

    //Update check status
    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            if(PIECE_AT(i, j) == wK) {
                currentState.whiteInCheck = isAttacked(j, i, BLACK);
            }
            else if(PIECE_AT(i, j) == bK) {
                currentState.blackInCheck = isAttacked(j, i, WHITE);
            }
        }
    }

    currentState.turn = (Colour)-currentState.turn;
    ++currentState.turns;

    //Update hash
    hash_combine(currentState.hashCode, m.fromX);
    hash_combine(currentState.hashCode, m.toX);
    hash_combine(currentState.hashCode, m.fromY);
    hash_combine(currentState.hashCode, m.toY);
    hash_combine(currentState.hashCode, (int)m.promotion);
}

void Game::undoLastMove() {
    currentState = stateHistory.top();
    stateHistory.pop();
}

//TODO: This is #1 performance bottleneck
const bool Game::isAttacked(int x, int y, const Colour attackingColour) {
    if(pieceColours[currentState.board[y][x]] == attackingColour) {
        //Cannot attack own side
        return false;
    }

    //Pawns
    const int pawnMoveDir = attackingColour == BLACK ? 1 : -1;

    const Piece leftPawnDiag = PIECE_AT(y - pawnMoveDir, x - 1);
    const Piece rightPawnDiag = PIECE_AT(y - pawnMoveDir, x + 1);

    if(pieceColours[leftPawnDiag] == attackingColour && pieceTypes[leftPawnDiag] == Pawn) {
        return true;
    }

    if(pieceColours[rightPawnDiag] == attackingColour && pieceTypes[rightPawnDiag] == Pawn) {
        return true;
    }

    //En passant
    if(y == currentState.enPass.y - pawnMoveDir && x == currentState.enPass.x) {
        const Piece leftEnPassant = PIECE_AT(y, x - 1);
        const Piece rightEnPassant = PIECE_AT(y, x + 1);

        if(pieceColours[leftEnPassant] == attackingColour && pieceTypes[leftEnPassant] == Pawn) {
            return true;
        }

        if(pieceColours[rightEnPassant] == attackingColour && pieceTypes[rightEnPassant] == Pawn) {
            return true;
        }
    }

    //King
    const Piece adjacentKings[8] = {
        leftPawnDiag,
        rightPawnDiag,
        PIECE_AT(y + pawnMoveDir, x - 1),
        PIECE_AT(y + pawnMoveDir, x + 1),
        PIECE_AT(y, x -1),
        PIECE_AT(y, x + 1),
        PIECE_AT(y - 1, x),
        PIECE_AT(y + 1, x)
    };

    for(int i = 0; i < 8; ++i) {
        const Piece p = adjacentKings[i];
        if(pieceColours[p] == attackingColour && pieceTypes[p] == King) {
            return true;
        }
    }
    
    //Knights
    const Piece adjacentKnights[8] = {
        PIECE_AT(y - 1, x - 2),
        PIECE_AT(y - 2, x - 1),
        PIECE_AT(y - 1, x + 2),
        PIECE_AT(y - 2, x + 1),
        PIECE_AT(y + 1, x - 2),
        PIECE_AT(y + 2, x - 1),
        PIECE_AT(y + 1, x + 2),
        PIECE_AT(y + 2, x + 1)
    };

    for(int i = 0; i < 8; ++i) {
        const Piece p = adjacentKnights[i];
        if(pieceColours[p] == attackingColour && pieceTypes[p] == Knight) {
            return true;
        }
    }

    //Bishop/Queen
    int yDirsDiag[2] = { -1, 1 };
    int xDirsDiag[2] = { -1, 1 };
    for(int i = 0; i < 2; ++i) {
        for(int j = 0; j < 2; ++j) {
            int searchDirY = yDirsDiag[i];
            int searchDirX = xDirsDiag[j];
            int searchY = y;    
            int searchX = x;
            Piece p = empty;
            do {
                searchX += searchDirX;
                searchY += searchDirY;
                p = PIECE_AT(searchY, searchX);
                if(p != empty && p != off_board) {
                    const PieceType type = pieceTypes[p];
                    if(pieceColours[p] == attackingColour && (type == Bishop || type == Queen)) {
                        return true;
                    }
                }
            } while(p == empty);
        }
    }

    //Rook/Queen    
    int yDirsNonDiag[4] = { -1, 1, 0, 0 };
    int xDirsNonDiag[4] = { 0, 0, -1, 1 };
    for(int i = 0; i < 4; ++i) {
        int searchDirY = yDirsNonDiag[i];
        int searchDirX = xDirsNonDiag[i];
        int searchY = y;
        int searchX = x;
        Piece p = empty;
        do {
            searchX += searchDirX;
            searchY += searchDirY;
            p = PIECE_AT(searchY, searchX);
            if(p != empty && p != off_board) {
                const PieceType type = pieceTypes[p];
                if(pieceColours[p] == attackingColour && (type == Rook || type == Queen)) {
                    return true;
                }
            }
        } while(p == empty);
    }

    return false;
}

void Game::addQuietMove(const Colour turn, std::vector<move>& moves, move move) {
    move.score = 0;
    moves.push_back(move);
}

void Game::addCaptureMove(const Colour turn, std::vector<move>& moves, move move, const Piece pieceMoved, const Piece capturedPiece) {
    if(pieceTypes[capturedPiece] == King) {
        return;
    }
    
    move.score = MvvLVA[pieceTypes[capturedPiece]][pieceTypes[pieceMoved]];
    moves.push_back(move);
}

void Game::generateMoves(const Colour turn, std::vector<move>& moves) {
    for(int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Piece p = currentState.board[i][j];

            if(pieceColours[p] != turn) {
                continue;
            }

            switch(pieceTypes[p]) {
                case Pawn: 
                    generatePawnMoves(turn, moves, j, i);
                    break;
                case Knight:
                    generateKnightMoves(turn, moves, j, i);
                    break;
                case Bishop:
                    generateBishopMoves(turn, moves, j, i);
                    break;
                case Rook:
                    generateRookMoves(turn, moves, j, i);
                    break;
                case Queen:
                    generateQueenMoves(turn, moves, j, i);
                    break;  
                case King:
                    generateKingMoves(turn, moves, j, i);
                    break;
                default:
                    break;
            }
        }
    }
}

void Game::generatePawnMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    Piece piece = PIECE_AT(y, x);

    const int pawnMoveDir = turn == BLACK ? 1 : -1;

    const Piece leftDiag = PIECE_AT(y + pawnMoveDir, x - 1);
    const Piece rightDiag = PIECE_AT(y + pawnMoveDir, x + 1);

    if((currentState.enPass.x == x - 1 && currentState.enPass.y == y + pawnMoveDir) || pieceColours[leftDiag] == -turn) {
        addCaptureMove(turn, moves, {
            .fromX = x,
            .fromY = y,
            .toX = x - 1,
            .toY = y + pawnMoveDir
        }, piece, leftDiag);
    }

    if((currentState.enPass.x == x + 1 && currentState.enPass.y == y + pawnMoveDir) || pieceColours[rightDiag] == -turn) {
        addCaptureMove(turn, moves, {
            .fromX = x,
            .fromY = y,
            .toX = x + 1,
            .toY = y + pawnMoveDir
        }, piece, rightDiag);
    }

    const Piece forwardOne = PIECE_AT(y + pawnMoveDir, x);

    if(forwardOne == empty) {
        if(turn == WHITE && y == 1) {
            for(int i = 0; i < 4; ++i) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = x,
                    .toY = y + pawnMoveDir,
                    .promotion = promotionTypes[i]
                });
            }
        }
        else if(turn == BLACK && y == 6) {
            for(int i = 0; i < 4; ++i) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = x,
                    .toY = y + pawnMoveDir,
                    .promotion = promotionTypes[i]
                });
            }
        }
        else {
            addQuietMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x,
                .toY = y + pawnMoveDir
            });
        }

        const Piece forwardTwo = PIECE_AT(y + (pawnMoveDir * 2), x);

        if(((turn == BLACK && y == 1) || (turn == WHITE && y == 7)) && forwardTwo == empty) {
            addQuietMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x,
                .toY = y + (pawnMoveDir * 2)
            });
        }
    }
}

void Game::generateKnightMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    Piece piece = PIECE_AT(y, x);

    const int offsets[8][2] = {
        { -1, -2 },
        { -2, -1 },
        { -1, +2 },
        { -2, +1 },
        { +1, -2 },
        { +2, -1 },
        { +1, +2 },
        { +2, +1 }
    };

    for(int i = 0; i < 8; ++i) {
        Piece p = PIECE_AT(y + offsets[i][0], x + offsets[i][1]);

        if(p == empty) {
            addQuietMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x + offsets[i][1],
                .toY = y + offsets[i][0]
            });
        }
        else if(pieceColours[p] == -turn) {
            addCaptureMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x + offsets[i][1],
                .toY = y + offsets[i][0]
            }, piece, p);
        }
    }
}

void Game::generateBishopMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    Piece piece = PIECE_AT(y, x);

    int yDirsDiag[2] = { -1, 1 };
    int xDirsDiag[2] = { -1, 1 };

    for(int i = 0; i < 2; ++i) {
        for(int j = 0; j < 2; ++j) {
            int searchDirY = yDirsDiag[i];
            int searchDirX = xDirsDiag[j];
            int searchY = y;    
            int searchX = x;

            while(true) {
                searchX += searchDirX;
                searchY += searchDirY;
                Piece p = PIECE_AT(searchY, searchX);

                if(p == off_board || pieceColours[p] == turn) {
                    break;
                }

                if(p == empty) {
                    addQuietMove(turn, moves, {
                        .fromX = x,
                        .fromY = y,
                        .toX = searchX,
                        .toY = searchY
                    });
                }
                else if(pieceColours[p] == -turn) {
                    addCaptureMove(turn, moves, {
                        .fromX = x,
                        .fromY = y,
                        .toX = searchX,
                        .toY = searchY
                    }, piece, p);
                    break;
                }
            };
        }
    }
}

void Game::generateRookMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    Piece piece = PIECE_AT(y, x);

    int yDirsNonDiag[4] = { -1, 1, 0, 0 };
    int xDirsNonDiag[4] = { 0, 0, -1, 1 };
    for(int i = 0; i < 4; ++i) {
        int searchDirY = yDirsNonDiag[i];
        int searchDirX = xDirsNonDiag[i];
        int searchY = y;
        int searchX = x;

        while(true) {
            searchX += searchDirX;
            searchY += searchDirY;
            Piece p = PIECE_AT(searchY, searchX);

            if(p == off_board || pieceColours[p] == turn) {
                break;
            }

            if(p == empty) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = searchX,
                    .toY = searchY
                });
            }
            else if(pieceColours[p] == -turn) {
                addCaptureMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = searchX,
                    .toY = searchY
                }, piece, p);
                break;
            }
        };
    }
}

void Game::generateQueenMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    generateBishopMoves(turn, moves, x, y);
    generateRookMoves(turn, moves, x, y);
}

void Game::generateKingMoves(const Colour turn, std::vector<move>& moves, int x, int y) {
    Piece piece = PIECE_AT(y, x);

    const int offsets[8][2] = {
        { -1,  0 },
        { -1, -1 },
        {  0, -1 },
        {  1, -1 },
        {  1,  0 },
        {  1,  1 },
        {  0,  1 },
        { -1,  1 }
    };

    for(int i = 0; i < 8; ++i) {
        Piece p = PIECE_AT(y + offsets[i][1], x + offsets[i][0]);

        if(p == empty && !isAttacked(x + offsets[i][0], y + offsets[i][1], (Colour)-turn)) {
            addQuietMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x + offsets[i][0],
                .toY = y + offsets[i][1]
            });
        }
        else if(p != off_board && pieceColours[p] == -turn && !isAttacked(x + offsets[i][0], y + offsets[i][1], (Colour)-turn)) {
            addCaptureMove(turn, moves, {
                .fromX = x,
                .fromY = y,
                .toX = x + offsets[i][0],
                .toY = y + offsets[i][1]
            }, piece, p);
        }
    }

    //Castling
    if(turn == WHITE) {
        if(currentState.castlePerm & K) {
            int pathPositions[2][2] = {
                { 7, 5 },
                { 7, 6 }
            };

            bool canCastle = true;
            
            for(int i = 0; i < 2; ++i) {
                Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);
                
                if(p != empty || isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                    canCastle = false;
                }
            }

            if(canCastle) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = 6,
                    .toY = 7
                });
            }
        }

        if((currentState.castlePerm & Q) >> 1) {
            int pathPositions[3][2] = {
                { 7, 3 },
                { 7, 2 },
                { 7, 1 },
            };

            bool canCastle = true;

            for(int i = 0; i < 3; ++i) {
                Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);

                if(p != empty) {
                    canCastle = false;
                }
            }

            for(int i = 0; i < 2; ++i) {
                if(isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                    canCastle = false;
                }
            }

            if(canCastle) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = 2,
                    .toY = 7
                });
            }
        }
    }
    else {
        if((currentState.castlePerm & k) >> 2) {
            int pathPositions[2][2] = {
                { 0, 5 },
                { 0, 6 }
            };

            bool canCastle = true;
            
            for(int i = 0; i < 2; ++i) {
                Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);
                
                if(p != empty || isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                    canCastle = false;
                }
            }

            if(canCastle) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = 6,
                    .toY = 0
                });
            }
        }

        if((currentState.castlePerm & q) >> 3) {
            int pathPositions[3][2] = {
                { 0, 3 },
                { 0, 2 },
                { 0, 1 },
            };

            bool canCastle = true;

            for(int i = 0; i < 3; ++i) {
                Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);

                if(p != empty) {
                    canCastle = false;
                }
            }

            for(int i = 0; i < 2; ++i) {
                if(isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                    canCastle = false;
                }
            }

            if(canCastle) {
                addQuietMove(turn, moves, {
                    .fromX = x,
                    .fromY = y,
                    .toX = 2,
                    .toY = 0
                });
            }
        }
    }
}

int Game::getScore(int turn, const std::vector<move>& availableMyMoves, const std::vector<move>& availableOpponentMoves) {
    int score = 0;

    //Material score
    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            Piece p = PIECE_AT(i, j);

            switch(p) {
                case bP: 
                    score += 100;
                    score += (((float)i - 1.0) / 6.0) * 30; //max 1/2 pawn value score for being on opposite side
                    break;
                case bN: score += 300; break;
                case bB: score += 300; break;
                case bR: score += 500; break;
                case bQ: score += 900; break;
                case bK: score += 100000; break;
                case wP: 
                    score -= 100; 
                    score -= ((6.0 - (float)i) / 6.0) * 30; //max 1/2 pawn value score for being on opposite side
                    break;
                case wN: score -= 300; break;
                case wB: score -= 300; break;
                case wR: score -= 500; break;
                case wQ: score -= 900; break;
                case wK: score -= 100000; break;
                default: break;
            }
        }
    }

    //Mobility score
    score += -turn * (availableMyMoves.size() - availableOpponentMoves.size());

    //Protect king

    //Sentries

    //Queens,Rooks to 7th/1st

    return score;
}

void Game::print() {
    for(int i = 0; i < 8; ++i) {
        printf("%d  ", 8 - i);
        for(int j = 0; j < 8; ++j) {
            printf("%c ", getPieceDisplay(currentState.board[i][j]));
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n");
    printf("\n");
    printf("Turn: %s\n", currentState.turn == WHITE ? "WHITE" : "BLACK");
    // printf("Fifty: %d\n", fiftyMove);
    printf("En-passant: %d, %d\n", currentState.enPass.x, currentState.enPass.y);
    printf("Castling: K:%d Q:%d k:%d q:%d", (currentState.castlePerm & K), (currentState.castlePerm & Q) >> 1, (currentState.castlePerm & k) >> 2, (currentState.castlePerm & q) >> 3);
    printf("Hash code: %zu\n", currentState.hashCode);
    printf("\n");
}