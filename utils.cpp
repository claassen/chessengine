#include <chrono>

#include "game.h"
#include "utils.h"
#include "debug.h"

void split(const std::string& input, std::vector<std::string>& words) {
    std::istringstream iss(input);

    std::string word;
    while(iss >> word) {
        words.push_back(word);
    }
}

char getPieceDisplay(Piece piece) {
    switch(piece) {
        case empty: return '.';
        case bP: return 'p';
        case bN: return 'n';
        case bB: return 'b';
        case bR: return 'r';
        case bQ: return 'q';
        case bK: return 'k';
        case wP: return 'P';
        case wN: return 'N';
        case wB: return 'B';
        case wR: return 'R';
        case wQ: return 'Q';
        case wK: return 'K';
        default: return '?';
    }
}

const PieceType getPiecePromotionType(const char c) {
    switch(c) {
        case 'p': return Pawn;
        case 'n': return Knight;
        case 'b': return Bishop;
        case 'r': return Rook;
        case 'q': return Queen;
        default: return Empty;
    }
}

char getPiecePromotionChar(PieceType pieceType) {
    switch(pieceType) {
        case Pawn: return 'p';
        case Knight: return 'n';
        case Bishop: return 'b';
        case Rook: return 'r';
        case Queen: return 'q';
        default: return '\0';
    }
}

const Piece getPiece(const char c) {
    switch(c) {
        case 'p': return bP;
        case 'n': return bN;
        case 'b': return bB;
        case 'r': return bR;
        case 'q': return bQ;
        case 'k': return bK;
        case 'P': return wP;
        case 'N': return wN;
        case 'B': return wB;
        case 'R': return wR;
        case 'Q': return wQ;
        case 'K': return wK;
        default: return empty;
    }
}

const Piece getPiece(const char c, const Colour turn) {
    switch(c) {
        case 'p': return turn == BLACK ? bP : wP;
        case 'n': return turn == BLACK ? bN : wN;
        case 'b': return turn == BLACK ? bB : wB;
        case 'r': return turn == BLACK ? bR : wR;
        case 'q': return turn == BLACK ? bQ : wQ;
        default: return empty;
    }
}

const Piece getPiece(PieceType pieceType, const Colour turn) {
    switch(pieceType) {
        case Pawn: return turn == BLACK ? bP : wP;
        case Knight: return turn == BLACK ? bN : wN;
        case Bishop: return turn == BLACK ? bB : wB;
        case Rook: return turn == BLACK ? bR : wR;
        case Queen: return turn == BLACK ? bQ : wQ;
        default: return empty;
    }
}

const Colour getTurn(const char c) {
    switch(c) {
        case 'w': return WHITE;
        case 'b': return BLACK;
    }

    LOG("Unknown turn character");
    exit(1);
}

const CastlePerm getCastlePerm(const char c) {
    switch(c) {
        case 'K': return K;
        case 'Q': return Q;
        case 'k': return k;
        case 'q': return q;
    }

    LOG("Unknown castleperm character");
    exit(1);
}

const enPassLocation getEnPassLocation(const char c1, const char c2) {
    return EN_PASS((unsigned int)(c1 - 'a'), (unsigned int)(7 - (c2 - '0' - 1)));
}

const std::string getMoveStr(const move& move) {
    std::stringstream ss;
    ss << (char)('a' + move.fromX) << (8 - move.fromY) << (char)('a' + move.toX) << (8 - move.toY) << getPiecePromotionChar(move.promotion);
    return ss.str();
}

const move getMove(const std::string& moveStr) {
    PieceType promotion = Empty;

    if(moveStr.length() > 4) {
        promotion = getPiecePromotionType(moveStr.at(4));
    }

    unsigned int fromX = moveStr.at(0) - 'a';
    unsigned int fromY = 7 - (moveStr.at(1) - '0' - 1);
    unsigned int toX = moveStr.at(2) - 'a';
    unsigned int toY = 7 - (moveStr.at(3) - '0' - 1);

    return {  
        fromX,
        fromY,
        toX,
        toY,
        promotion
    };
}

const long long getCurrentTimeInMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
