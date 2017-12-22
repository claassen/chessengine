#ifndef LOGGING_H
#define LOGGING_H

//Define to disable debugging
#define NDEBUG

#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#ifndef NDEBUG
    #define INIT_LOGGING() (initializeLogging())
    #define LOG(message) (log(message))
    #define LOG_INPUT(message) (logInput(message))
#else
    #define INIT_LOGGING()
    #define LOG(message)
    #define LOG_INPUT(message)
#endif

void initializeLogging();
void log(std::string message);
void logInput(std::string message);

#endif