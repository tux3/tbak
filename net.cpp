#include "net.h"
#include "netsock.h"
#include "server.h"
#include "serialize.h"
#include <algorithm>
#include <iostream>

Net::Net()
{

}

PublicKey Net::getNodePk(NetAddr nodeAddr)
{
    PublicKey pk{0};

    NetSock node{nodeAddr};
    node.send({NetPacketType::GetPk});
    std::vector<char> rawData = node.recvPacket().data;
    auto it = rawData.cbegin();
    pk = ::deserializeConsume<PublicKey>(it);

    return pk;
}

void Net::sendPacket(NetPacket packet, NetAddr nodeAddr)
{
    NetSock sock{std::move(nodeAddr)};
    if (sock.isConnected())
        sock.send(packet.serialize());
}

bool Net::sendAuth(const NetSock& sock, const Server& server)
{
    NetPacket packet{NetPacketType::Auth, ::serialize(server.getPublicKey())};
    sock.send(packet);
    NetPacket reply = sock.recvPacket();
    return reply.type == NetPacketType::Auth;
}
