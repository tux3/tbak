#ifndef SERVER_H
#define SERVER_H

#include "netsock.h"
#include "crypto.h"
#include <atomic>

class NodeDB;
class FolderDB;

/// Server node class.
class Server
{
public:
    /// Will use configFilePath to store and load permanent server data
    Server(const std::string& configFilePath, NodeDB& ndb, FolderDB& fdb);

    int exec(); ///< Enters the server's event loop. Blocks until the server exits.

    const PublicKey& getPublicKey() const;
    const SecretKey& getSecretKey() const;

protected:
    void load(const std::string& path);
    void deserialize(const std::vector<char>& data);
    void save(const std::string& path) const;
    std::vector<char> serialize() const;

    void handleClient(NetSock& client);

private:
    // Server commands
    void cmdGetPk(NetSock& client);
    bool cmdAuth(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdFolderStats(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdFolderCreate(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdFolderList(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdDownloadArchive(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdDownloadArchiveMetadata(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdUploadArchive(NetSock& client, NetPacket& packet, PublicKey& remoteKey);
    bool cmdDeleteArchive(NetSock& client, NetPacket& packet, PublicKey& remoteKey);

private:
    NetSock insock;
    SecretKey sk;
    PublicKey pk;
    NodeDB& ndb;
    FolderDB& fdb;

public:
    static std::atomic<bool> abortall; ///< If set to true, the server will return form its event loop
};

#endif // SERVER_H
