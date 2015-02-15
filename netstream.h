#ifndef NETSTREAM_H
#define NETSTREAM_H

#include <vector>
#include "netpacket.h"

class NetStream
{
public:
    NetStream();
    NetStream(std::vector<char> rawData);
    std::vector<char> getData();

private:
    std::vector<NetPacket> packets;
};

#endif // NETSTREAM_H
