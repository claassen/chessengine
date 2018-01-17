#ifndef TCPSOCKET_H
#define TCPSOCKET_H

class TCPSocket {
private:
    FILE *fin, *fout;
public:
    TCPSocket(const std::string& address, int port);
    std::string readLine();
    void writeLine(const std::string& s);
};

#endif