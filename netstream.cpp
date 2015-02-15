#include "netstream.h"

using namespace std;

NetStream::NetStream()
{
}

NetStream::NetStream(std::vector<char> rawData)
{
    std::vector<char>::const_iterator it = rawData.cbegin();
    while (it != rawData.cend())
        packets.emplace_back(NetPacket::deserialize(it));
}

vector<char> NetStream::getData()
{
    vector<char> data;
    for (const NetPacket& packet : packets)
        data.insert(end(packet.data), begin(packet.data), end(packet.data));
    return data;
}

