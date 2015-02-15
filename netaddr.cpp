#include "netaddr.h"
#include "settings.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <stdexcept>

NetAddr::NetAddr(const std::string &uri)
{
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    int r = getaddrinfo(uri.c_str(), PORT_NUMBER_STR, &hints, &sockaddr);
    if (r < 0)
    {
        sockaddr = 0;
        throw std::runtime_error("NetAddr::NetAddr: getaddrinfo failed");
    }

}

NetAddr::NetAddr(NetAddr &&other)
{
    sockaddr = other.sockaddr;
    other.sockaddr = nullptr;
}

NetAddr::~NetAddr()
{
    freeaddrinfo(sockaddr);
}

