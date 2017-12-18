#ifndef UTILS_H
#define UTILS_H

#include <sstream>
#include <string>
#include <vector>

#include "game.h"

void split(const std::string& input, std::vector<std::string>& words);
char getPieceDisplay(Piece piece);
const PieceType getPiecePromotionType(const char c);
char getPiecePromotionChar(PieceType pieceType);
const Colour getColour(Piece p);
const PieceType getPieceType(const Piece p);
const Piece getPiece(const char c);
const Piece getPiece(const char c, const Colour turn);
const Piece getPiece(PieceType pieceType, const Colour turn);
const Colour getTurn(const char c);
const CastlePerm getCastlePerm(const char c);
const enPassLocation getEnPassLocation(const char c1, const char c2);
const std::string getMoveStr(const move& move);

#endif