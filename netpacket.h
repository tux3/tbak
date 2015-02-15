#ifndef NETPACKET_H
#define NETPACKET_H

#include <vector>
#include <cstdint>
#include "netsock.h"

enum class NetPacketType : uint8_t
{
    // Non-authenticated packets
    Abort, ///< There was an error, abort all pending operations on this socket
    GetPk, ///< Request/send a node's public key
    Auth, ///< Perform authentication, necessary to use authenticated packets

    // Authenticated packets
    FolderSourceReload, ///< Ask the server to reload a source folder from disk
    FolderStats, ///< Request/send a folder's main statistics
    FolderTimeList, ///< List of file names and last acess time of this folder's files
    DownloadArchiveFile, ///< Fetch file compressed then encrypted-with-source-key from source or archive folder
};

class NetPacket
{
public:
    NetPacket()=default;
    NetPacket(NetPacketType type);
    NetPacket(NetPacketType type, std::vector<char> data);
    std::vector<char> serialize() const;
    static NetPacket deserialize(std::vector<char>::const_iterator& data);
    static NetPacket deserialize(const NetSock &clientsock); ///< May rethrow exceptions from the socket

public:
    NetPacketType type;
    std::vector<char> data;
};

#endif // NETPACKET_H
