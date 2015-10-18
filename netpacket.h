#ifndef NETPACKET_H
#define NETPACKET_H

#include <vector>
#include <cstdint>
#include "netsock.h"

class NetPacket
{
public:
    enum Type : uint8_t
    {
        // Non-authenticated packets
        Abort, ///< There was an error, abort all pending operations on this socket
        GetPk, ///< Request/send a node's public key
        Auth, ///< Perform authentication, necessary to use authenticated packets

        // Authenticated packets
        FolderCreate, ///< Ask the remote server to do a "folder add-archive" of our local folder
        FolderStats, ///< Request/send a folder's main statistics
        FolderList, ///< List of file names and last acess time of this folder's files
        DownloadArchive, ///< Fetch compressed/encrypted file from an archive folder
        DownloadArchiveMetadata, ///< Fetch the metadata of a compressed/encrypted file from an archive folder
        UploadArchive, ///< Send compressed/encrypted file to an archive folder
        DeleteArchive, ///< Requests that the server deletes a file from its archive folder
    };

public:
    NetPacket()=default;
    NetPacket(NetPacket::Type type);
    NetPacket(NetPacket::Type type, std::vector<char> data);
    std::vector<char> serialize() const;
    static NetPacket deserialize(std::vector<char>::const_iterator& data);
    static NetPacket deserialize(const NetSock &clientsock); ///< May rethrow exceptions from the socket

public:
    NetPacket::Type type;
    std::vector<char> data;
};

#endif // NETPACKET_H
