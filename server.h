#ifndef SERVER_H
#define SERVER_H

#include "netsock.h"
#include "crypto.h"
#include <atomic>
#include <mutex>

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
    NetSock insock;
    SecretKey sk;
    PublicKey pk;
    NodeDB& ndb;
    FolderDB& fdb;
    std::mutex coarselock; ///< Big lock of the server, for easy multithreading

public:
    static std::atomic<bool> abortall; ///< If set to true, the server will return form its event loop
};

#endif // SERVER_H
