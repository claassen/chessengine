#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>

#ifndef NDEBUG
    #define LOG(message) (log(message))
    #define LOG_INPUT(message) (logInput(message))
#else
    #define LOG(message)
    #define LOG_INPUT(message)
#endif

void initialize();
void log(std::string message);
void logInput(std::string message);

#endif