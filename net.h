#ifndef NET_H
#define NET_H

#include "crypto.h"
#include "netpacket.h"
#include "netaddr.h"

/// Handles low and high level networking
class Net
{
public:
    Net();

/// Tries to fetch the public key of a node, returns a zeroed pk on error
static PublicKey getNodePk(NetAddr nodeAddr);

static void sendPacket(NetPacket packet, NetAddr nodeAddr);
static void sendPacket(NetPacket packet, NetSock sock);

static bool sendAuth(const NetSock& sock, const Server& server);

private:

};

#endif // NET_H
