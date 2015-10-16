#include "server.h"
#include "netpacket.h"
#include "serialize.h"
#include "nodedb.h"
#include "folderdb.h"
#include "compression.h"
#include <iostream>
#include <algorithm>

using namespace std;

void Server::cmdGetPk(NetSock &client)
{
    cout << "Public key requested" << endl;
    client.send(NetPacket(NetPacketType::GetPk, ::serialize(pk)));
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
            client.send(NetPacket{NetPacketType::Auth});
            remoteKey = node.getPk();
            return true;
        }
    }

    cout << "Auth failed"<<endl;
    client.send(NetPacket{NetPacketType::Abort});
    return false;
}

bool Server::cmdFolderStats(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    auto pit = packet.data.cbegin();
    string path = ::deserializeConsume<string>(pit);
    std::cout<<"Folder stats requested for "<<path<<endl;
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&path](const Folder& f){return f.getPath()==path;});
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        return false;
    }
    else
    {
        NetPacket packet{NetPacketType::FolderStats, fit->serialize()};
        Crypto::encryptPacket(packet, *this, remoteKey);
        client.send(packet);
    }
    return true;
}

bool Server::cmdFolderPush(NetSock& client, NetPacket& packet, PublicKey&)
{
    Folder f(packet.data);
    std::cout<<"Folder push requested for "<<f.getPath()<<endl;

    f.setType(FolderType::Archive);
    f.open(true);
    f.close();
    fdb.addFolder(f);
    client.send({NetPacketType::FolderPush});
    return true;
}

bool Server::cmdFolderSourceReload(NetSock& client, NetPacket& packet, PublicKey&)
{
    auto pit = packet.data.cbegin();
    string path = ::deserializeConsume<string>(pit);
    std::cout<<"Folder reload requested for "<<path<<endl;
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&path](const Folder& f)
    {
        return f.getPath()==path && f.getType() == FolderType::Source;
    });
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        return false;
    }
    else
    {
        Folder& folder = fdb.getFolder(fit);
        folder.open(true);
        folder.close();
        client.send({NetPacketType::FolderSourceReload});
    }
    return true;
}

bool Server::cmdFolderTimeList(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    auto pit = packet.data.cbegin();
    string path = ::deserializeConsume<string>(pit);
    std::cout<<"Folder time list requested for "<<path<<endl;
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&path](const Folder& f){return f.getPath()==path;});
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        return false;
    }
    else
    {
        vector<char> data;
        Folder& folder = fdb.getFolder(fit);
        folder.open();
        for (const File& file : folder.getFiles())
        {
            ::serializeAppend(data, file.getPathHash());
            ::serializeAppend(data, file.getAttrs().mtime);
        }
        data = Compression::deflate(data);
        NetPacket packet{NetPacketType::FolderTimeList, data};
        Crypto::encryptPacket(packet, *this, remoteKey);
        client.send(packet);
    }
    return true;
}

bool Server::cmdDownloadArchiveFile(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    auto pit = packet.data.cbegin();
    string folderPath = ::deserializeConsume<string>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);
    string filePath = filePathHash.toBase64();
    cout << "Download request in "<<folderPath<<" of "<<filePath<<endl;

    // Find folder
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&folderPath](const Folder& f){return f.getPath()==folderPath;});
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        cout << "Requested folder not found, sending Abort"<<endl;
        return false;
    }

    // Find file
    Folder& folder = fdb.getFolder(fit);
    folder.open();
    const std::vector<File>& files = folder.getFiles();
    bool found = false;
    const File* file = nullptr;
    for (const File& f : files)
    {
        if (f.getPathHash() == filePathHash)
        {
            file = &f;
            found = true;
            break;
        }
    }
    if (!found)
    {
        client.send({NetPacketType::Abort});
        cout << "Requested file not found, sending Abort"<<endl;
        return false;
    }
    vector<char> fdata;

    // Compress and encrypt if needed
    if (folder.getType() == FolderType::Source)
    {
        File archived = *file;
        vector<char> content = file->readAll();
        content = Compression::deflate(content);
        Crypto::encrypt(content, *this, remoteKey);
        archived.setActualSize(archived.metadataSize() + content.size());
        fdata = archived.serialize();
        copy(move_iterator<vector<char>::iterator>(begin(content)),
             move_iterator<vector<char>::iterator>(end(content)),
             back_inserter(fdata));
    }
    else
    {
        fdata = file->serialize();
        vectorAppend(fdata, file->readAll());
    }

    // Send result
    NetPacket reply{NetPacketType::DownloadArchiveFile, fdata};
    Crypto::encryptPacket(reply, *this, remoteKey);
    client.send(reply);
    return true;
}

bool Server::cmdUploadArchiveFile(NetSock& client, NetPacket& packet, PublicKey& remoteKey)
{
    auto pit = packet.data.cbegin();
    string folderpath = ::deserializeConsume<string>(pit);

    // Find folder
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&folderpath](const Folder& f)
    {
        return f.getPath()==folderpath && f.getType() == FolderType::Archive;
    });
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        cout << "Requested folder not found, sending Abort"<<endl;
        return false;
    }

    vector<char> data(pit, packet.data.cend());
    auto it = data.cbegin();
    File fmeta(&*fit, it);
    cout << "Upload request in "<<folderpath<<" of "<<fmeta.getPathHash().toBase64()<<endl;
    fdb.getFolder(fit).writeArchiveFile(data, *this, remoteKey);
    client.send({NetPacketType::UploadArchiveFile});
    return true;
}

bool Server::cmdDeleteArchiveFile(NetSock& client, NetPacket& packet, PublicKey&)
{
    auto pit = packet.data.cbegin();
    string folderPath = ::deserializeConsume<string>(pit);
    PathHash filePathHash = ::deserializeConsume<PathHash>(pit);
    string filePath = filePathHash.toBase64();

    // Find folder
    const vector<Folder>& folders = fdb.getFolders();
    auto fit = find_if(begin(folders), end(folders), [&folderPath](const Folder& f)
    {
        return f.getPath()==folderPath && f.getType() == FolderType::Archive;
    });
    if (fit == end(folders))
    {
        client.send({NetPacketType::Abort});
        cout << "Requested folder not found, sending Abort"<<endl;
        return false;
    }

    // Remove file
    cout << "Removal request in "<<folderPath<<" of "<<filePath<<endl;
    Folder& folder = fdb.getFolder(fit);
    folder.open();
    folder.removeArchiveFile(filePathHash);
    client.send({NetPacketType::DeleteArchiveFile});
    return true;
}
