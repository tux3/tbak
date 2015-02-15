#include "server.h"
#include "net.h"
#include "netpacket.h"
#include "settings.h"
#include "serialize.h"
#include "nodedb.h"
#include "folderdb.h"
#include "compression.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

using namespace std;

std::atomic<bool> Server::abortall{false};

Server::Server(const std::string& configFilePath, NodeDB &ndb, FolderDB &fdb)
    : ndb{ndb}, fdb{fdb}
{
    load(configFilePath);
}

void Server::save(const string &path) const
{
    vector<char> data = serialize();
    ofstream f(path, ios_base::binary | ios_base::trunc);
    if (f.is_open())
    {
        copy(begin(data), end(data), ostreambuf_iterator<char>(f));
        f.close();
    }
}

vector<char> Server::serialize() const
{
    vector<char> data;

    serializeAppend(data, sk);
    serializeAppend(data, pk);

    return data;
}

void Server::load(const std::string& path)
{
    ifstream f(path, ios_base::binary);
    vector<char> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (f.is_open())
        f.close();

    if (!data.empty())
    {
        deserialize(data);
    }
    else
    {
        Crypto::genkeys(pk, sk);
        save(path);
    }
}

void Server::deserialize(const std::vector<char> &data)
{
    auto it = begin(data);

    sk = deserializeConsume<SecretKey>(it);
    pk = deserializeConsume<PublicKey>(it);
}

int Server::exec()
{
    if (!insock.listen())
    {
        cerr << "Server::exec: Couldn't listen on port "<<PORT_NUMBER_STR<<endl;
        return -1;
    }

    for (;;)
    {
        if (abortall)
            return 0;

        NetSock client = insock.accept();
        handleClient(client);
    }

    return 0;
}

const PublicKey& Server::getPublicKey() const
{
    return pk;
}

const SecretKey& Server::getSecretKey() const
{
    return sk;
}

void Server::handleClient(NetSock& client)
{
    bool authenticated = false;
    PublicKey remoteKey;

    for (;;)
    {
        try
        {
            if (abortall)
                return;

            if (client.isShutdown())
            {
                cout << "Client disconnected"<<endl;
                break;
            }
            NetPacket packet = NetPacket::deserialize(client);

            // Unauthenticated packets
            if (packet.type == NetPacketType::GetPk)
            {
                cout << "Public key requested" << endl;
                client.send(NetPacket(NetPacketType::GetPk, ::serialize(pk)));
            }
            else if (packet.type == NetPacketType::Auth)
            {
                bool found = false;
                if (packet.data.size() == sizeof(PublicKey))
                {
                    auto nodes = ndb.getNodes();
                    for (const Node& node : nodes)
                    {
                        if (memcmp(&node.getPk()[0], &packet.data[0], sizeof(PublicKey)))
                            continue;

                        cout << "Auth successful"<<endl;
                        client.send(NetPacket{NetPacketType::Auth});
                        remoteKey = node.getPk();
                        authenticated = true;
                        found = true;
                    }
                }
                if (!found)
                {
                    cout << "Auth failed"<<endl;
                    client.send(NetPacket{NetPacketType::Abort});
                    break;
                }
            }
            else if (!authenticated)
            {
                cerr << "Unauthenticated packet of type "<<(int)packet.type<<" with size "<<packet.data.size()<<" received"<<endl;
                client.send({NetPacketType::Abort});
                break;
            }
            else // Authenticated packets
            {
                Crypto::decryptPacket(packet, *this, remoteKey);

                if (packet.type == NetPacketType::FolderStats)
                {
                    auto pit = packet.data.cbegin();
                    string path = ::deserializeConsume<string>(pit);
                    std::cout<<"Folder stats requested for "<<path<<endl;
                    const vector<Folder>& folders = fdb.getFolders();
                    auto fit = find_if(begin(folders), end(folders), [&path](const Folder& f){return f.getPath()==path;});
                    if (fit == end(folders))
                    {
                        client.send({NetPacketType::Abort});
                        break;
                    }
                    else
                    {
                        NetPacket packet{NetPacketType::FolderStats, fit->serialize()};
                        Crypto::encryptPacket(packet, *this, remoteKey);
                        client.send(packet);
                    }
                }
                else if (packet.type == NetPacketType::FolderTimeList)
                {
                    auto pit = packet.data.cbegin();
                    string path = ::deserializeConsume<string>(pit);
                    std::cout<<"Folder time list requested for "<<path<<endl;
                    vector<Folder>& folders = fdb.getFolders();
                    auto fit = find_if(begin(folders), end(folders), [&path](const Folder& f){return f.getPath()==path;});
                    if (fit == end(folders))
                    {
                        client.send({NetPacketType::Abort});
                        break;
                    }
                    else
                    {
                        vector<char> data;
                        fit->open(true);
                        for (const File& file : fit->getFiles())
                        {
                            ::serializeAppend(data, file.path);
                            ::serializeAppend(data, file.attrs.mtime);
                        }
                        fit->close();
                        data = Compression::deflate(data);
                        NetPacket packet{NetPacketType::FolderTimeList, data};
                        Crypto::encryptPacket(packet, *this, remoteKey);
                        client.send(packet);
                    }
                }
                else if (packet.type == NetPacketType::DownloadArchiveFile)
                {
                    auto pit = packet.data.cbegin();
                    string folderpath = ::deserializeConsume<string>(pit);
                    string filepath = ::deserializeConsume<string>(pit);
                    cout << "Download archive file request: "<<folderpath<<", "<<filepath<<endl;

                    // Find folder
                    vector<Folder>& folders = fdb.getFolders();
                    auto fit = find_if(begin(folders), end(folders), [&folderpath](const Folder& f){return f.getPath()==folderpath;});
                    if (fit == end(folders))
                    {
                        client.send({NetPacketType::Abort});
                        break;
                    }

                    // Find file
                    Folder& folder = *fit;
                    folder.open();
                    const std::vector<File>& files = folder.getFiles();
                    folder.close();
                    bool found = false;
                    const File* file = nullptr;
                    for (const File& f : files)
                    {
                        if (f.path == filepath)
                        {
                            file = &f;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        client.send({NetPacketType::Abort});
                        break;
                    }
                    vector<char> fdata;

                    // Compress and encrypt if needed
                    if (folder.getType() == FolderType::Source)
                    {
                        File archived = *file;
                        vector<char> content = file->readAll();
                        content = Compression::deflate(content);
                        Crypto::encrypt(content, *this, remoteKey);
                        archived.actualSize = archived.metadataSize() + content.size();
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
                }
                else
                {
                    cerr << "Unknown packet of type "<<(int)packet.type<<" with size "<<packet.data.size()<<" received"<<endl;
                    client.send({NetPacketType::Abort});
                }
            }
        }
        catch (...)
        {
            cout << "Server::handleClient: Caught exception, returning"<<endl;
            break;
        }
    }
}
