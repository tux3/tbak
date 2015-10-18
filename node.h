#ifndef NODE_H
#define NODE_H

#include <vector>
#include <string>
#include <array>

#include "crypto.h"
#include "filetime.h"

class NetSock;
class Server;
class SourceFile;

class Node
{
public:
    Node(const std::string& uri, PublicKey pk);  ///< Add node with this URI
    Node(const std::vector<char>& data); ///< Construct from serialized data

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);

    const std::string& getUri() const;
    std::string getPkString() const;
    const PublicKey& getPk() const;

    // RPC calls
    bool createFolder(const NetSock& sock, const Server& s, const PathHash& folder) const;
    std::vector<FileTime> fetchFolderList(const NetSock& sock, const Server& s, const PathHash& folder) const;
    void uploadFileAsync(const NetSock& sock, const Server& s, const PathHash& folder, const SourceFile& file) const;
    void deleteFileAsync(const NetSock& sock, const Server& s, const PathHash& folder, const PathHash& file) const;
    std::vector<char> downloadFileMetadata(const NetSock& sock, const Server& s,
                                           const PathHash& folder, const PathHash& file) const;
    std::vector<char> downloadFile(const NetSock& sock, const Server& s,
                                   const PathHash& folder, const PathHash& file) const;

private:
    std::string uri;
    PublicKey pk;
};

#endif // NODE_H
