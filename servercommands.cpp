#include "server.h"
#include "net/netpacket.h"
#include "serialize.h"
#include "nodedb.h"
#include "folderdb.h"
#include "compression.h"
#include "util/humanreadable.h"
#include <iostream>
#include <algorithm>

using namespace std;

void Server::cmdGetPk(NetSock &client)
{
    cout << "Public key requested" << endl;
    client.send(NetPacket(NetPacket::GetPk, ::serialize(pk)));
}

bool Server::cmdAuth(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    if (packet.data.size() == sizeof(PublicKey))
    {
        auto nodes = ndb.getNodes();
        for (const Node& node : nodes)
        {
            if (!equal(&packet.data[0], &packet.data[0]+sizeof(PublicKey), &node.getPk()[0]))
                continue;

            cout << "Auth successful"<<endl;
            client.send(NetPacket{NetPacket::Auth});
            remoteKey = node.getPk();
            return true;
        }
    }

    cout << "Auth failed"<<endl;
    client.send(NetPacket{NetPacket::Abort});
    return false;
}

bool Server::cmdFolderStats(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    auto pit = packet.data.cbegin();
    PathHash pathHash = ::deserializeConsume<PathHash>(pit);

    Archive* archive = fdb.getArchive(pathHash);
    if (!archive)
    {
        std::cout<<"cmdFolderStats: No such folder "<<pathHash.toBase64()<<endl;
        client.send({NetPacket::Abort});
        return false;
    }

    std::cout<<"Folder stats requested for "<<pathHash.toBase64()<<endl;
    vector<char> data = ::serialize(archive->getActualSize());
    client.sendEncrypted({NetPacket::FolderStats, data}, *this, remoteKey);
    return true;
}

bool Server::cmdFolderCreate(NetSock& client, NetPacket& packet, PublicKey&)
{
    if (packet.data.size() != PathHash::hashlen)
    {
        std::cout << "Server::cmdFolderCreate: Received invalid data, aborting"<<endl;
        return false;
    }
    PathHash pathHash((uint8_t*)packet.data.data());
    std::cout<<"Folder creation requested for "<<pathHash.toBase64()<<endl;
    fdb.addArchive(pathHash);
    client.send({NetPacket::FolderCreate});
    return true;
}

bool Server::cmdFolderList(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    if (packet.data.size() != PathHash::hashlen)
    {
        std::cout << "Server::cmdFolderList: Received invalid data, aborting"<<endl;
        return false;
    }
    PathHash pathHash((uint8_t*)packet.data.data());

    std::cout<<"Folder time list requested for "<<pathHash.toBase64()<<endl;
    Archive* archive = fdb.getArchive(pathHash);
    if (!archive)
    {
        client.send({NetPacket::Abort});
        return false;
    }

    vector<char> data;
    for (const ArchiveFile& file : archive->getFiles())
    {
        ::serializeAppend(data, file.getPathHash());
        ::serializeAppend(data, file.getMtime());
    }
    data = Compression::deflate(data);
    client.sendEncrypted({NetPacket::FolderList, data}, *this, remoteKey);
    return true;
}

bool Server::cmdDownloadArchive(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    if (packet.data.size() != 2*PathHash::hashlen)
    {
        cout << "Server::cmdDownloadArchive: Received invalid data, aborting"<<endl;
        return false;
    }
    auto pit = packet.data.cbegin();
    PathHash folderPathHash = ::deserializeConsume<PathHash>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);

    // Find folder
    Archive* archive = fdb.getArchive(folderPathHash);
    if (!archive)
    {
        client.send({NetPacket::Abort});
        cout << "cmdDownloadArchive: Requested folder "<<folderPathHash.toBase64()<<" not found"<<endl;
        return false;
    }

    // Find file
    ArchiveFile* file = archive->getFile(filePathHash);
    if (!file)
    {
        client.send({NetPacket::Abort});
        cout << "cmdDownloadArchive: Requested file "<<filePathHash.toBase64()
             <<" in folder "<<folderPathHash.toBase64()<<" not found"<<endl;
        return false;
    }

    vector<char> fdata = ::serialize(file->getMtime());
    vectorAppend(fdata, file->readAll());
    cout << "Download request in "<<folderPathHash.toBase64()<<" of "<<filePathHash.toBase64()
         <<" ("<<humanReadableSize(file->getActualSize())<<')'<<endl;
    client.sendEncrypted({NetPacket::DownloadArchive, fdata}, *this, remoteKey);
    return true;
}

bool Server::cmdDownloadArchiveMetadata(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    if (packet.data.size() != 2*PathHash::hashlen)
    {
        cout << "Server::cmdDownloadArchiveMetadata: Received invalid data, aborting"<<endl;
        return false;
    }
    auto pit = packet.data.cbegin();
    PathHash folderPathHash = ::deserializeConsume<PathHash>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);

    // Find folder
    Archive* archive = fdb.getArchive(folderPathHash);
    if (!archive)
    {
        client.send({NetPacket::Abort});
        cout << "cmdDownloadArchiveMetadata: Requested folder "<<folderPathHash.toBase64()<<" not found"<<endl;
        return false;
    }

    // Find file
    ArchiveFile* file = archive->getFile(filePathHash);
    if (!file)
    {
        client.send({NetPacket::Abort});
        cout << "cmdDownloadArchiveMetadata: Requested file "<<filePathHash.toBase64()
             <<" in folder "<<folderPathHash.toBase64()<<" not found"<<endl;
        return false;
    }
    vector<char> data = file->readMetadata();
    if (!data.size())
    {
        cout << "cmdDownloadArchiveMetadata: Failed to read from file "<<filePathHash.toBase64()
             <<" in folder "<<folderPathHash.toBase64()<<endl;
        client.send({NetPacket::Abort});
        return false;
    }
    cout << "File size is "<<file->getActualSize()<<endl;
    serializeAppend(data, file->getActualSize());
    cout << "Metadata download request in "<<folderPathHash.toBase64()<<" of "<<filePathHash.toBase64()<<endl;
    client.sendEncrypted({NetPacket::DownloadArchiveMetadata, data}, *this, remoteKey);
    return true;
}

bool Server::cmdUploadArchive(NetSock& client, NetPacket& packet, PublicKey&)
{
    if (packet.data.size() < 2*PathHash::hashlen+sizeof(uint64_t))
    {
        cout << "Server::cmdUploadArchive: Received invalid data, aborting"<<endl;
        return false;
    }
    auto pit = packet.data.cbegin();
    PathHash folderPathHash = ::deserializeConsume<PathHash>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);
    uint64_t mtime = ::deserializeConsume<uint64_t>(pit);

    // Find folder
    Archive* a = fdb.getArchive(folderPathHash);
    if (!a)
    {
        cout << "cmdUploadArchive: Folder "<<folderPathHash.toBase64()<<" not found"<<endl;
        client.send({NetPacket::Abort});
        return false;
    }

    vector<char> data(pit, packet.data.cend());
    cout << "Upload request in "<<folderPathHash.toBase64()<<" of "<<filePathHash.toBase64()
         <<" ("<<humanReadableSize(data.size())<<')'<<endl;
    a->writeArchiveFile(filePathHash, mtime, data);
    client.send({NetPacket::UploadArchive});
    return true;
}

bool Server::cmdDeleteArchive(NetSock& client, NetPacket& packet, PublicKey&)
{
    if (packet.data.size() != 2*PathHash::hashlen)
    {
        cout << "Server::cmdDeleteArchive: Received invalid data, aborting"<<endl;
        return false;
    }
    auto pit = packet.data.cbegin();
    PathHash folderPathHash = ::deserializeConsume<PathHash>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);

    // Find folder
    Archive* archive = fdb.getArchive(folderPathHash);
    if (!archive)
    {
        client.send({NetPacket::Abort});
        cout << "Requested folder not found, sending Abort"<<endl;
        return false;
    }

    // Remove file
    if (archive->removeArchiveFile(filePathHash))
    {
        cout << "Removal request in "<<folderPathHash.toBase64()<<" of "<<filePathHash.toBase64()<<endl;
        client.send({NetPacket::DeleteArchive});
        return true;
    }
    else
    {
        cout << "Failed removal request in "<<folderPathHash.toBase64()<<" of "<<filePathHash.toBase64()
             <<", file not found"<<endl;
        return false;
    }
}
