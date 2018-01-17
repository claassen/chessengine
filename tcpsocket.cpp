#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>

#include "tcpsocket.h"

#define MAXLINE 2048

TCPSocket::TCPSocket(const std::string& address, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1) {
        throw std::runtime_error("Failed creating socket.");
    }

    if(inet_addr(address.c_str()) == -1) {
        throw std::runtime_error("Invalid address.");
    }

    sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(address.c_str());
    server.sin_family = AF_INET;    
    server.sin_port = htons(port);

    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        throw std::runtime_error("Error connecting.");    
    }

    fin = fdopen(sockfd, "r");
    fout = fdopen(sockfd, "w");

    setvbuf(fin, NULL, _IOLBF, 1024);
    setvbuf(fout, NULL, _IOLBF, 1024);
}

std::string TCPSocket::readLine() {
    char line[MAXLINE];
    fgets(line, MAXLINE, fin);

    std::string s = line;

    if (!s.empty() && s[s.length()-1] == '\n') {
        s.erase(s.length()-1);
    }

    return s;
}

void TCPSocket::writeLine(const std::string& s) {
    std::cout << "SENDING: " << s;
    fputs(s.c_str(), fout);
    fflush(fout);
}