#ifndef NETSOCK_H
#define NETSOCK_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "crypto.h"

class NetAddr;
class NetPacket;
class Server;

/// Abstraction of a network socket
class NetSock
{
public:
    NetSock(); ///< Only constructs a default socket
    NetSock(const NetAddr& addr); ///< Constructs a socket and connect to this address
    NetSock(NetSock&& other);
    ~NetSock();
    NetSock& operator=(NetSock&& other);

    bool listen();
    NetSock accept();
    std::vector<char> recv(int count = -1) const; ///< Receives data until count are received, or unable to receive more
    uint8_t recvByte() const;
    NetPacket recvPacket() const;
    NetPacket recvEncryptedPacket(const Server& s, const PublicKey& pk) const; ///< Returns the decrypted packet
    uint8_t peekByte() const;
    bool isPacketAvailable() const;
    size_t bytesAvailable() const;

    bool connect(const NetAddr& addr);
    bool connect(const std::string& uri);
    bool isConnected();
    bool isShutdown();
    void send(const NetPacket& packet) const;
    void sendEncrypted(NetPacket& packet, const Server& s, const PublicKey& pk) const; ///< Modifies the packet inplace!
    void sendEncrypted(NetPacket&& packet, const Server& s, const PublicKey& pk) const;

    /// Encrypts packet, sends it, receives a reply and returns it decrypted
    NetPacket secureRequest(NetPacket& packet, const Server& s, const PublicKey& pk) const;
    NetPacket secureRequest(NetPacket&& packet, const Server& s, const PublicKey& pk) const;

protected:
    NetSock(int sockfd, bool connected=false);

private:
    NetSock(const NetSock& other) = delete;
    NetSock& operator=(const NetSock&) = delete;

private:
    int sockfd;
    bool connected;
};

#endif // NETSOCK_H
