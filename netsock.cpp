#include "netsock.h"
#include "netpacket.h"
#include "netaddr.h"
#include "settings.h"
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <system_error>
#include <iostream>

using namespace std;

NetSock::NetSock()
    : sockfd{0}, connected{false}
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
}

NetSock::NetSock(const NetAddr& addr)
    : NetSock()
{
    connect(addr);
}

NetSock::NetSock(int sockfd, bool connected)
    : sockfd{sockfd}, connected{connected}
{
}

NetSock::NetSock(NetSock&& other)
{
    sockfd = other.sockfd;
    connected = other.connected;
    other.connected = false;
    other.sockfd = -1;
}

NetSock::~NetSock()
{
    //cout << "Socket "<<sockfd <<" closed"<<endl;
    close(sockfd);
}

bool NetSock::connect(const NetAddr& addr)
{
    int result = ::connect(sockfd, addr.sockaddr->ai_addr, addr.sockaddr->ai_addrlen);
    connected = (result >= 0);
    return connected;
}

bool NetSock::isConnected()
{
    return connected;
}

void NetSock::send(const std::vector<char>& data) const
{
    //cout << "Sending "<<data.size() <<" bytes\n";
    if (connected)
    {
        int r =::write(sockfd, &data[0], data.size());
        if (r<0)
            throw runtime_error("NetSock::send: Write failed");
        else if ((unsigned)r!=data.size())
            throw runtime_error("NetSock::send: Failed to send all data");
    }
    else
    {
        throw std::runtime_error("NetSock::send: Not connected");
    }
}

void NetSock::send(const NetPacket& packet) const
{
    send(packet.serialize());
}

bool NetSock::listen()
{
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NUMBER);
    if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
        return false;
    ::listen(sockfd,5);
    return true;
}

NetSock NetSock::accept()
{
    sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = ::accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    return NetSock(newsockfd, true);
}

std::vector<char> NetSock::recv(int count) const
{
    std::vector<char> data;

    if (!count)
        return data;

    const size_t bufsize = 4096;
    char* buf = new char[bufsize];
    unsigned totalReceived = 0;
    int result;

    do {
        unsigned toReceive;
        if (count == -1)
            toReceive = bufsize;
        else
            toReceive = min((size_t)(count-totalReceived), bufsize);
        result = ::read(sockfd, buf, toReceive);
        if (result <= 0)
        {
            perror("NetSock::recv");
            break;
        }
        totalReceived += result;
        data.insert(end(data), buf, buf+result);
    } while (totalReceived < (unsigned)count);

    delete[] buf;
    return data;
}

uint8_t NetSock::recvByte() const
{
    uint8_t byte;
    int result = ::read(sockfd, &byte, 1);
    if (result == 0)
        throw std::runtime_error("NetSock::recvByte: Remote host closed connection");
    else if (result < 0)
        throw std::runtime_error("NetSock::recvByte: Error reading from socket");
    else
        return byte;
}

NetPacket NetSock::recvPacket() const
{
    return NetPacket::deserialize(*this);
}

bool NetSock::isShutdown()
{
    pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLRDHUP | POLLHUP | POLLIN;
    poll(&fds, 1, 1000);
    return (fds.revents & POLLRDHUP) | (fds.revents & POLLHUP);
}

bool NetSock::isPacketAvailable() const
{
    /// Read vuint packet size from socket
    unsigned char num3;
    size_t size = 0;
    int num2 = 0;
    unsigned i=0;
    do
    {
        if (bytesAvailable() < i+1)
            return false;
        num3 = peekByte(); i++;
        size |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);

    if (bytesAvailable() < i+size)
        return false;
    else
        return true;
}

size_t NetSock::bytesAvailable() const
{
    size_t size;
    if (ioctl(sockfd, FIONREAD, &size) < 0)
        throw runtime_error("NetSock::bytesAvailable: ioctl FIONREAD failure");

    return size;
}

uint8_t NetSock::peekByte() const
{
    uint8_t byte;
    int result = ::recv(sockfd, &byte, 1, MSG_PEEK);
    if (result == 0)
        throw std::runtime_error("NetSock::peekByte: Remote host closed connection");
    else if (result < 0)
        throw std::runtime_error("NetSock::peekByte: Error reading from socket");
    else
        return byte;
}
