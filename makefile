all:
	g++ -g -std=c++11 -Wall testengine.cpp game.cpp search.cpp pvtable.cpp utils.cpp logging.cpp -o testengine

