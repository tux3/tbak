#include "node.h"
#include "serialize.h"
#include "net/netsock.h"
#include "net/netpacket.h"
#include "compression.h"
#include "sourcefile.h"
#include "server.h"
#include <iostream>

using namespace std;

Node::Node(const std::string& Uri, PublicKey Pk)
    : uri{Uri}
{
    pk = Pk;
}

Node::Node(const std::vector<char>& data)
{
    deserialize(data);
}

std::vector<char> Node::serialize() const
{
    vector<char> data;

    serializeAppend(data, uri);
    serializeAppend(data, pk);

    return data;
}

void Node::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    uri = deserializeConsume<decltype(uri)>(it);
    pk = deserializeConsume<decltype(pk)>(it);
}

const std::string& Node::getUri() const
{
    return uri;
}

std::string Node::getPkString() const
{
    return Crypto::keyToString(pk);
}

const PublicKey& Node::getPk() const
{
    return pk;
}

bool Node::createFolder(const NetSock &sock, const Server &s, const PathHash &folder) const
{
    return sock.secureRequest({NetPacket::FolderCreate, ::serialize(folder)}, s, pk).type == NetPacket::FolderCreate;
}

std::vector<FileTime> Node::fetchFolderList(const NetSock &sock, const Server &s, const PathHash &folder) const
{
    // Try to get the content list of the folder
    vector<char> rFilesData;
    {
        NetPacket reply = sock.secureRequest({NetPacket::FolderList, ::serialize(folder)}, s, pk);
        if (reply.type != NetPacket::FolderList)
            throw runtime_error("Unable to get folder list from node "+getUri()+", giving up\n");
        rFilesData = Compression::inflate(reply.data);
    }

    static constexpr int entrySize = PathHash::hashlen + sizeof(uint64_t);
    if (rFilesData.size() % entrySize != 0)
        throw runtime_error("Received invalid data from node "+getUri()+", giving up\n");

    vector<FileTime> rEntries;
    rEntries.reserve(rFilesData.size()/entrySize);
    auto it = rFilesData.cbegin();
    while (it != rFilesData.cend())
    {
        FileTime e;
        e.hash = ::deserializeConsume<PathHash>(it);
        e.mtime = ::deserializeConsume<uint64_t>(it);
        rEntries.push_back(move(e));
    }
    return rEntries;
}

bool Node::uploadFile(const NetSock &sock, const Server &s, const PathHash &folder, const SourceFile &file) const
{
    vector<char> data;
    serializeAppend(data, folder);
    serializeAppend(data, file.getPathHash());
    serializeAppend(data, file.getAttrs().mtime);
    {
        // Encrypt the metadata and contents separately, so we can later download the metadata only
        vector<char> fileData;
        vector<char> meta = file.serializeMetadata();
        Crypto::encrypt(meta, s, s.getPublicKey());
        vectorAppend(fileData, vuintToData(meta.size()));
        vectorAppend(fileData, move(meta));
        vector<char> contents = Compression::deflate(file.readAll());
        Crypto::encrypt(contents, s, s.getPublicKey());
        vectorAppend(fileData, move(contents));
        vectorAppend(data, move(fileData));
    }
    return sock.secureRequest({NetPacket::UploadArchive, data}, s, pk).type == NetPacket::UploadArchive;
}

bool Node::deleteFile(const NetSock &sock, const Server &s, const PathHash &folder, const PathHash &file) const
{
    vector<char> data;
    serializeAppend(data, folder);
    serializeAppend(data, file);
    return sock.secureRequest({NetPacket::DeleteArchive, data}, s, pk).type == NetPacket::DeleteArchive;
}

std::vector<char> Node::downloadFileMetadata(const NetSock &sock, const Server &s, const PathHash &folder, const PathHash &file) const
{
    vector<char> data;
    serializeAppend(data, folder);
    serializeAppend(data, file);
    NetPacket reply = sock.secureRequest({NetPacket::DownloadArchiveMetadata, data}, s, pk);
    if (reply.type != NetPacket::DownloadArchiveMetadata)
        throw runtime_error("Node::downloadFileMetadata: Download failed");
    return reply.data;
}

std::vector<char> Node::downloadFile(const NetSock &sock, const Server &s, const PathHash &folder, const PathHash &file) const
{
    vector<char> data;
    serializeAppend(data, folder);
    serializeAppend(data, file);
    NetPacket reply = sock.secureRequest({NetPacket::DownloadArchive, data}, s, pk);
    if (reply.type != NetPacket::DownloadArchive)
        throw runtime_error("Node::downloadFile: Download failed");
    return reply.data;
}
