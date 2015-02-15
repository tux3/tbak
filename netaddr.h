#ifndef NETADDR_H
#define NETADDR_H

#include <string>
#include <netdb.h>

class NetAddr
{
public:
    NetAddr(const std::string& uri);
    NetAddr(NetAddr&& other);
    ~NetAddr();

private:
    NetAddr(const NetAddr& other) = delete;
    NetAddr& operator=(const NetAddr&) = delete;

public:
    struct addrinfo* sockaddr;
};

#endif // NETADDR_H
