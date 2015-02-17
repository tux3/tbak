#ifndef NETSOCK_H
#define NETSOCK_H

#include <vector>
#include <cstdint>

class NetAddr;
class NetPacket;

/// Abstraction of a network socket
class NetSock
{
public:
    NetSock(); ///< Only constructs a default socket
    NetSock(const NetAddr& addr); ///< Constructs a socket and connect to this address
    NetSock(NetSock&& other);
    ~NetSock();

    bool isConnected();

    bool listen();
    NetSock accept();
    std::vector<char> recv(int count = -1) const; ///< Receives data until count are received, or unable to receive more
    uint8_t recvByte() const; ///< Throws std::runtime_error on error
    NetPacket recvPacket() const; ///< Throws std::runtime_error on error

    bool connect(const NetAddr& addr);
    bool isShutdown();
    void send(const std::vector<char>& data) const;
    void send(const NetPacket& packet) const;

protected:
    NetSock(int sockfd, bool connected=false);

public:
    NetSock(const NetSock& other) = delete;
    NetSock& operator=(const NetSock&) = delete;

private:
    int sockfd;
    bool connected;
};

#endif // NETSOCK_H
