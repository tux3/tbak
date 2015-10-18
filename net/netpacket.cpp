#include "net/netpacket.h"
#include "serialize.h"

NetPacket::NetPacket(Type type)
    : type{type}
{
}

NetPacket::NetPacket(NetPacket::Type type, std::vector<char> data)
    : type{type}, data{data}
{
}

std::vector<char> NetPacket::serialize() const
{
    std::vector<char> rawData;
    serializeAppend(rawData, (uint8_t)type);
    vectorAppend(rawData, vuintToData(data.size()));
    vectorAppend(rawData, data);
    return rawData;
}

NetPacket NetPacket::deserialize(std::vector<char>::const_iterator& data)
{
    NetPacket packet;
    packet.type = (NetPacket::Type)deserializeConsume<uint8_t>(data);
    size_t size = dataToVUint(data);
    packet.data.reserve(size);
    packet.data.insert(end(packet.data), data, data+size);
    data += size;
    return packet;
}

NetPacket NetPacket::deserialize(const NetSock& clientsock)
{
    NetPacket packet;
    packet.type = (NetPacket::Type)clientsock.recvByte();

    /// Read vuint packet size from socket
    unsigned char num3;
    size_t size = 0;
    int num2 = 0;
    int i=0;
    do
    {
        num3 = clientsock.recvByte(); i++;
        size |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);

    packet.data = clientsock.recv(size);
    return packet;
}
