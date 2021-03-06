#include <cstdlib>
#include <sstream>

#include "game.h"
#include "zobrist.h"
#include "utils.h"
#include "debug.h"

#define SET_PIECE(row, col, piece) (currentState.board[row + 2][col + 2] = piece)
#define PIECE_AT(row, col)         (currentState.board[row + 2][col + 2])

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
        { 0, 99, 99, 99, 99, 99, 99 }, //king
};

void Game::startPosition(const std::string& fen) {
    stateHistory = std::vector<gameState>();

    currentState.turn = WHITE;
    currentState.castlePerm = 0;
    currentState.enPass = NO_EN_PASS;

    std::vector<std::string> parts;
    split(fen, parts);

    //Initialize off_board squares
    for(int i = 0; i < 12; ++i) {
        currentState.board[0][i] = off_board;
        currentState.board[1][i] = off_board;
        currentState.board[10][i] = off_board;
        currentState.board[11][i] = off_board;
        currentState.board[i][0] = off_board;
        currentState.board[i][1] = off_board;
        currentState.board[i][10] = off_board;
        currentState.board[i][11] = off_board;
    }

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
                SET_PIECE(y, x, empty);
                ++x;
            }
        }
        else if(c != ' ') {
            SET_PIECE(y, x, getPiece(c));
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
        currentState.enPass = getEnPassLocation(enPass.at(0), enPass.at(1));
    }

    //Fifty move
    currentState.fiftyMove = std::stoi(parts[4]);

    //Turns
    currentState.turns = std::stoi(parts[5]);

    //Set hash code
    currentState.hashCode = 0;

    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            Piece p = PIECE_AT(row, col);

            if(p != empty) {
                currentState.hashCode ^= zobrist::pieceHashes[row][col][p];
            }
        }
    }

    currentState.hashCode ^= zobrist::enPassHashes[currentState.enPass.y][currentState.enPass.x];
    currentState.hashCode ^= zobrist::castlePermHashes[currentState.castlePerm];
    currentState.hashCode ^= zobrist::turnHashes[currentState.turn == WHITE ? 0 : 1];
}

void Game::makeMove(const move& m) {
    stateHistory.push_back(currentState);

    //Hash update
    currentState.hashCode ^= zobrist::enPassHashes[currentState.enPass.y][currentState.enPass.x];
    currentState.hashCode ^= zobrist::castlePermHashes[currentState.castlePerm];
    currentState.hashCode ^= zobrist::turnHashes[currentState.turn == WHITE ? 0 : 1];

    const Piece p = PIECE_AT(m.fromY, m.fromX);
    const Piece capturedPiece = PIECE_AT(m.toY, m.toX);

    //Don't actually care about 50 move rule
    // if(p == bP || p == wP || PIECE_AT(m.toY, m.toX) != empty || (currentState.enPass != NO_EN_PASS && m.toX == currentState.enPass.x && m.toY == currentState.enPass.y)) {
    //     //Pawn move or capture, reset 50 move rule
    //     currentState.fiftyMove = 0;
    // }
    // else {
    //     ++currentState.fiftyMove;
    // }

    //Remove captured piece from hash
    if(capturedPiece != empty) {
        currentState.hashCode ^= zobrist::pieceHashes[m.toY][m.toX][capturedPiece];
    }

    //Move piece
    if(m.promotion != Empty) {
        Piece promotion = getPiece(m.promotion, currentState.turn);
        currentState.hashCode ^= zobrist::pieceHashes[m.toY][m.toX][promotion];    
        SET_PIECE(m.toY, m.toX, promotion);
    }
    else {
        currentState.hashCode ^= zobrist::pieceHashes[m.toY][m.toX][p];
        SET_PIECE(m.toY, m.toX, p);
    }

    //En passant capture
    if(currentState.enPass != NO_EN_PASS && pieceTypes[p] == Pawn && m.toX == currentState.enPass.x && m.toY == currentState.enPass.y) {
        if(currentState.turn == WHITE) {
            currentState.hashCode ^= zobrist::pieceHashes[currentState.enPass.y + 1][currentState.enPass.x][bP];
            SET_PIECE(currentState.enPass.y + 1, currentState.enPass.x, empty);
        }
        else {
            currentState.hashCode ^= zobrist::pieceHashes[currentState.enPass.y - 1][currentState.enPass.x][wP];
            SET_PIECE(currentState.enPass.y - 1, currentState.enPass.x, empty);
        }

        currentState.enPass = NO_EN_PASS;
    }

    //Remove piece from old location
    currentState.hashCode ^= zobrist::pieceHashes[m.fromY][m.fromX][p];
    SET_PIECE(m.fromY, m.fromX, empty);

    //Update castling permissions for captured rooks
    if(capturedPiece == bR) {
        if(m.toX == 0 && m.toY == 0) {
            currentState.castlePerm &= ~q;
        }
        else if(m.toX == 7 && m.toY == 0) {
            currentState.castlePerm &= ~k;
        }
    }
    else if(capturedPiece == wR) {
        if(m.toX == 0 && m.toY == 7) {
            currentState.castlePerm &= ~Q;
        }
        else if(m.toX == 7 && m.toY == 7) {
            currentState.castlePerm &= ~K;
        }
    }

    //Move rooks for castling
    if(p == bK && m.toX == m.fromX - 2) {
        //Black king side
        currentState.hashCode ^= zobrist::pieceHashes[0][m.toX + 1][bR];
        currentState.hashCode ^= zobrist::pieceHashes[0][0][bR];
        currentState.castlePerm &= ~q;
        SET_PIECE(0, m.toX + 1, bR);
        SET_PIECE(0, 0, empty);
    }
    else if(p == bK && m.toX == m.fromX + 2) {
        //Black queen side
        currentState.hashCode ^= zobrist::pieceHashes[0][m.toX - 1][bR];
        currentState.hashCode ^= zobrist::pieceHashes[0][7][bR];
        currentState.castlePerm &= ~k;
        SET_PIECE(0, m.toX - 1, bR);
        SET_PIECE(0, 7, empty);
    }
    else if(p == wK && m.toX == m.fromX - 2) {
        //White king side
        currentState.hashCode ^= zobrist::pieceHashes[7][m.toX + 1][wR];
        currentState.hashCode ^= zobrist::pieceHashes[7][0][wR];
        currentState.castlePerm &= ~Q;
        SET_PIECE(7, m.toX + 1, wR);
        SET_PIECE(7, 0, empty);
    }
    else if(p == wK && m.toX == m.fromX + 2) {
        //White queen side
        currentState.hashCode ^= zobrist::pieceHashes[7][m.toX - 1][wR];
        currentState.hashCode ^= zobrist::pieceHashes[7][7][wR];
        currentState.castlePerm &= ~K;
        SET_PIECE(7, m.toX - 1, wR);
        SET_PIECE(7, 7, empty);
    }

    //Reset en pasant
    currentState.enPass = NO_EN_PASS;

    //En passant
    if((p == bP || p == wP) && std::abs(m.fromY - m.toY) > 1) {
        currentState.enPass = EN_PASS(m.fromX, (unsigned int)(m.fromY < m.toY ? m.fromY + 1 : m.fromY - 1));
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
            Piece kingPiece = PIECE_AT(i, j);
            if(kingPiece == wK) {
                currentState.whiteInCheck = isAttacked(j, i, BLACK);
            }
            else if(kingPiece == bK) {
                currentState.blackInCheck = isAttacked(j, i, WHITE);
            }
        }
    }

    currentState.turn = (Colour)-currentState.turn;
    ++currentState.turns;

    //Hash
    currentState.hashCode ^= zobrist::enPassHashes[currentState.enPass.y][currentState.enPass.x];
    currentState.hashCode ^= zobrist::castlePermHashes[currentState.castlePerm];
    currentState.hashCode ^= zobrist::turnHashes[currentState.turn == WHITE ? 0 : 1];
}

void Game::undoLastMove() {
    currentState = stateHistory.back();
    stateHistory.pop_back();
}

const bool Game::isAttacked(unsigned int x, unsigned int y, const Colour attackingColour) {
    if(pieceColours[PIECE_AT(y, x)] == attackingColour) {
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
    if(currentState.enPass != NO_EN_PASS && y == (unsigned int)(currentState.enPass.y - pawnMoveDir) && x == currentState.enPass.x) {
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

void Game::addQuietMove(move_list& moveList, move move) {
    ASSERT(move.fromX >= 0 && move.fromY >= 0 && move.toX < 8 && move.toY < 8);

    moveList.addMove(move);
}

void Game::addCaptureMove(move_list& moveList, move move, const Piece pieceMoved, const Piece capturedPiece) {
    ASSERT(move.fromX >=0 && move.fromY >= 0 && move.toX < 8 && move.toY < 8);
    ASSERT(pieceMoved != empty);
    ASSERT(pieceMoved != off_board);
    ASSERT(capturedPiece != empty);
    ASSERT(pieceMoved != off_board);

    if(pieceTypes[capturedPiece] == King) {
        return;
    }
    
    move.score = MvvLVA[pieceTypes[capturedPiece]][pieceTypes[pieceMoved]];
    
    moveList.addMove(move);
}

void Game::generateMoves(move_list& moves, bool capturesOnly) {
    for(unsigned int i = 0; i < 8; i++) {
        for (unsigned int j = 0; j < 8; j++) {
            Piece p = PIECE_AT(i, j);

            if(pieceColours[p] != currentState.turn) {
                continue;
            }

            switch(pieceTypes[p]) {
                case Pawn: 
                    generatePawnMoves(moves, j, i, capturesOnly);
                    break;
                case Knight:
                    generateKnightMoves(moves, j, i, capturesOnly);
                    break;
                case Bishop:
                    generateBishopMoves(moves, j, i, capturesOnly);
                    break;
                case Rook:
                    generateRookMoves(moves, j, i, capturesOnly);
                    break;
                case Queen:
                    generateQueenMoves(moves, j, i, capturesOnly);
                    break;  
                case King:
                    generateKingMoves(moves, j, i, capturesOnly);
                    break;
                default:
                    break;
            }
        }
    }
}

void Game::generatePawnMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    const Colour turn = currentState.turn;

    Piece piece = PIECE_AT(y, x);

    const int pawnMoveDir = turn == BLACK ? 1 : -1;

    const Piece leftDiag = PIECE_AT(y + pawnMoveDir, x - 1);
    const Piece rightDiag = PIECE_AT(y + pawnMoveDir, x + 1);

    //En passant capture left
    if(currentState.enPass != NO_EN_PASS && currentState.enPass.x == x - 1 && currentState.enPass.y == y + pawnMoveDir) {
        const Piece enPassPiece = PIECE_AT(y, x - 1);

        ASSERT(enPassPiece != empty);

        addCaptureMove(moves, {
            x,
            y,
            x - 1,
            y + pawnMoveDir
        }, piece, enPassPiece);
    }
    //Regular capture left
    else if(pieceColours[leftDiag] == -turn) {
        ASSERT(leftDiag != empty);

        if(turn == WHITE && y == 1) {
            //Promotion white
            for(int i = 0; i < 4; ++i) {
                addCaptureMove(moves, {
                    x,
                    y,
                    x - 1,
                    y + pawnMoveDir,
                    promotionTypes[i]
                }, piece, leftDiag);
            }
        }
        else if(turn == BLACK && y == 6) {
            //Promotion black
            for(int i = 0; i < 4; ++i) {
                addCaptureMove(moves, {
                    x,
                    y,
                    x - 1,
                    y + pawnMoveDir,
                    promotionTypes[i]
                }, piece, leftDiag);
            }
        }
        else {
            addCaptureMove(moves, {
                x,
                y,
                x - 1,
                y + pawnMoveDir
            }, piece, leftDiag);
        }
    }

    //En passant capture right
    if(currentState.enPass != NO_EN_PASS && currentState.enPass.x == x + 1 && currentState.enPass.y == y + pawnMoveDir) {
        const Piece enPassPiece = PIECE_AT(y, x + 1);

        ASSERT(enPassPiece != empty);

        addCaptureMove(moves, {
            x,
            y,
            x + 1,
            y + pawnMoveDir
        }, piece, enPassPiece);
    }
    //Regular capture right
    else if(pieceColours[rightDiag] == -turn) {
        ASSERT(rightDiag != empty);

        if(turn == WHITE && y == 1) {
            for(int i = 0; i < 4; ++i) {
                addCaptureMove(moves, {
                    x,
                    y,
                    x + 1,
                    y + pawnMoveDir,
                    promotionTypes[i]
                }, piece, rightDiag);
            }
        }
        else if(turn == BLACK && y == 6) {
            for(int i = 0; i < 4; ++i) {
                addCaptureMove(moves, {
                    x,
                    y,
                    x + 1,
                    y + pawnMoveDir,
                    promotionTypes[i]
                }, piece, rightDiag);
            }
        }
        else {
            addCaptureMove(moves, {
                x,
                y,
                x + 1,
                y + pawnMoveDir
            }, piece, rightDiag);
        }
    }

    const Piece forwardOne = PIECE_AT(y + pawnMoveDir, x);

    if(!capturesOnly) {
        //Non capture move forward
        if(forwardOne == empty) {
            if(turn == WHITE && y == 1) {
                //White promotion
                for(int i = 0; i < 4; ++i) {
                    addQuietMove(moves, {
                        x,
                        y,
                        x,
                        y + pawnMoveDir,
                        promotionTypes[i]
                    });
                }
            }
            else if(turn == BLACK && y == 6) {
                //Black promotion
                for(int i = 0; i < 4; ++i) {
                    addQuietMove(moves, {
                        x,
                        y,
                        x,
                        y + pawnMoveDir,
                        promotionTypes[i]
                    });
                }
            }
            else {
                addQuietMove(moves, {
                    x,
                    y,
                    x,
                    y + pawnMoveDir
                });
            }

            const Piece forwardTwo = PIECE_AT(y + (pawnMoveDir * 2), x);

            //Double move forward from starting position
            if(forwardTwo == empty && ((turn == BLACK && y == 1) || (turn == WHITE && y == 6))) {
                addQuietMove(moves, {
                    x,
                    y,
                    x,
                    y + (pawnMoveDir * 2)
                });
            }
        }
    }
}

void Game::generateKnightMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    const Colour turn = currentState.turn;

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

        if(p == empty && !capturesOnly) {
            addQuietMove(moves, {
                x,
                y,
                x + offsets[i][1],
                y + offsets[i][0]
            });
        }
        else if(pieceColours[p] == -turn) {
            addCaptureMove(moves, {
                x,
                y,
                x + offsets[i][1],
                y + offsets[i][0]
            }, piece, p);
        }
    }
}

void Game::generateBishopMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    const Colour turn = currentState.turn;

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

                if(p == empty && !capturesOnly) {
                    addQuietMove(moves, {
                        x,
                        y,
                        (unsigned int)searchX,
                        (unsigned int)searchY
                    });
                }
                else if(pieceColours[p] == -turn) {
                    addCaptureMove(moves, {
                        x,
                        y,
                        (unsigned int)searchX,
                        (unsigned int)searchY
                    }, piece, p);
                    break;
                }
            };
        }
    }
}

void Game::generateRookMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    const Colour turn = currentState.turn;

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

            if(p == empty && !capturesOnly) {
                addQuietMove(moves, {
                    x,
                    y,
                    (unsigned int)searchX,
                    (unsigned int)searchY
                });
            }
            else if(pieceColours[p] == -turn) {
                addCaptureMove(moves, {
                    x,
                    y,
                    (unsigned int)searchX,
                    (unsigned int)searchY
                }, piece, p);
                break;
            }
        };
    }
}

void Game::generateQueenMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    generateBishopMoves(moves, x, y, capturesOnly);
    generateRookMoves(moves, x, y, capturesOnly);
}

void Game::generateKingMoves(move_list& moves, unsigned int x, unsigned int y, bool capturesOnly) {
    const Colour turn = currentState.turn;

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

        if(p == empty && !capturesOnly && !isAttacked(x + offsets[i][0], y + offsets[i][1], (Colour)-turn)) {
            addQuietMove(moves, {
                x,
                y,
                x + offsets[i][0],
                y + offsets[i][1]
            });
        }
        else if(p != off_board && pieceColours[p] == -turn && !isAttacked(x + offsets[i][0], y + offsets[i][1], (Colour)-turn)) {
            ASSERT(p != empty);

            addCaptureMove(moves, {
                x,
                y,
                x + offsets[i][0],
                y + offsets[i][1]    
            }, piece, p);
        }
    }

    if(!capturesOnly) {
        //Castling
        if(turn == WHITE) {
            if(currentState.castlePerm & K) {
                unsigned int pathPositions[3][2] = {
                    { 7, 4 },
                    { 7, 5 },
                    { 7, 6 }
                };

                bool canCastle = true;
                
                for(int i = 0; i < 3; ++i) {
                    Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);
                    
                    if((i != 0 && p != empty) || isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                        canCastle = false;
                        break;
                    }
                }

                if(canCastle) {
                    addQuietMove(moves, {
                        x,
                        y,
                        6,
                        7
                    });
                }
            }

            if((currentState.castlePerm & Q) >> 1) {
                unsigned int pathPositions[4][2] = {
                    { 7, 4 },
                    { 7, 3 },
                    { 7, 2 },
                    { 7, 1 },
                };

                bool canCastle = true;

                for(int i = 1; i < 4; ++i) {
                    Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);

                    if(p != empty) {
                        canCastle = false;
                        break;
                    }
                }

                if(canCastle) {
                    for(int i = 0; i < 3; ++i) {
                        if(isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                            canCastle = false;
                        }
                    }

                    if(canCastle) {
                        addQuietMove(moves, {
                            x,
                            y,
                            2,
                            7
                        });
                    }
                }
            }
        }
        else {
            if((currentState.castlePerm & k) >> 2) {
                unsigned int pathPositions[3][2] = {
                    { 0, 4 },
                    { 0, 5 },
                    { 0, 6 }
                };

                bool canCastle = true;
                
                for(int i = 0; i < 3; ++i) {
                    Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);
                    
                    if((i != 0 && p != empty) || isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                        canCastle = false;
                        break;
                    }
                }

                if(canCastle) {
                    addQuietMove(moves, {
                        x,
                        y,
                        6,
                        0
                    });
                }
            }

            if((currentState.castlePerm & q) >> 3) {
                unsigned int pathPositions[4][2] = {
                    { 0, 4 },
                    { 0, 3 },
                    { 0, 2 },
                    { 0, 1 },
                };

                bool canCastle = true;

                for(int i = 1; i < 4; ++i) {
                    Piece p = PIECE_AT(pathPositions[i][0], pathPositions[i][1]);

                    if(p != empty) {
                        canCastle = false;
                        break;
                    }
                }

                if(canCastle) {
                    for(int i = 0; i < 3; ++i) {
                        if(isAttacked(pathPositions[i][1], pathPositions[i][0], (Colour)-turn)) {
                            canCastle = false;
                        }
                    }

                    if(canCastle) {
                        addQuietMove(moves, {
                            x,
                            y,
                            2,
                            0
                        });
                    }
                }
            }
        }
    }
}

void Game::print() {
    for(int i = 0; i < 8; ++i) {
        printf("%d  ", 8 - i);
        for(int j = 0; j < 8; ++j) {
            printf("%c ", getPieceDisplay(PIECE_AT(i, j)));
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n");
    printf("\n");
    printf("Turn: %s\n", currentState.turn == WHITE ? "WHITE" : "BLACK");
    // printf("Fifty: %d\n", fiftyMove);
    printf("En-passant: %d, %d\n", currentState.enPass.x, currentState.enPass.y);
    printf("Castling: K:%d Q:%d k:%d q:%d\n", (currentState.castlePerm & K), (currentState.castlePerm & Q) >> 1, (currentState.castlePerm & k) >> 2, (currentState.castlePerm & q) >> 3);
    printf("Hash code: %llu\n", currentState.hashCode);
    printf("\n");
}

int Game::currentBoardValue() {
    int value = 0;

    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            Piece p = PIECE_AT(i, j);

            switch(p) {
                case bP: 
                case wP:                 
                    value += 100;    
                    break;
                case bN: 
                case wN:                 
                    value += 320; 
                    break;
                case bB: 
                case wB:                 
                    value += 330; 
                    break;
                case bR: 
                case wR:                 
                    value += 500; 
                    break;
                case bQ: 
                case wQ:                 
                    value += 900; 
                    break;
                default: 
                    break;
            }
        }
    }

    return value;
}