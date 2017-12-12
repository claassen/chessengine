#include "utils.h"
#include "logging.h"

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
        case wQ: return 'W';
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
        case 'p': return wP;
        case 'n': return wN;
        case 'b': return wB;
        case 'r': return wR;
        case 'q': return wQ;
        case 'k': return wK;
        case 'P': return bP;
        case 'N': return bN;
        case 'B': return bB;
        case 'R': return bR;
        case 'Q': return bQ;
        case 'K': return bK;
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

location getLocation(const char c1, const char c2) {
    return {
        .x = c1 - 'a',
        .y = 7 - (c2 - '0' - 1)
    };
}

const std::string getMoveStr(const int fromX, const int fromY, const int toX, const int toY, const PieceType promotion) {
    std::stringstream ss;
    ss << (char)('a' + fromX) << (8 - fromY) << (char)('a' + toX) << (8 - toY) << getPiecePromotionChar(promotion);
    return ss.str();
}