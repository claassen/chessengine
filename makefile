all:
	g++ -g -std=c++11 -Wall main.cpp game.cpp search.cpp zobrist.cpp pvtable.cpp evaluation.cpp utils.cpp debug.cpp perft.cpp -o testengine

