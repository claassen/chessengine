#include "debug.h"

std::ofstream __logFile("/Users/claassen/workspace/debug.txt");

void initializeLogging() {
    if(!__logFile.is_open()) {
        std::cout << "Failed to open debug output";
        exit(1);
    }
}

void log(std::string message) {
    __logFile << message << std::endl;
}

void logInput(std::string message) {
    log(std::string(">>") + message);
}

