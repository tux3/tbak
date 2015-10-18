#include "server.h"
#include "net/net.h"
#include "net/netpacket.h"
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
using ::NetPacket;

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
            if (packet.type == NetPacket::GetPk)
            {
                cmdGetPk(client);
            }
            else if (packet.type == NetPacket::Auth)
            {
                if (cmdAuth(client, packet, remoteKey))
                    authenticated = true;
                else
                    break;
            }
            else if (!authenticated)
            {
                cerr << "Unauthenticated packet of type "<<(int)packet.type<<" with size "<<packet.data.size()<<" received"<<endl;
                client.send({NetPacket::Abort});
                break;
            }
            else // Authenticated packets
            {
                Crypto::decryptPacket(packet, *this, remoteKey);

                if (packet.type == NetPacket::FolderStats)
                {
                    if (!cmdFolderStats(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::FolderCreate)
                {
                    if (!cmdFolderCreate(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::FolderList)
                {
                    if (!cmdFolderList(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::DownloadArchive)
                {
                    if (!cmdDownloadArchive(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::DownloadArchiveMetadata)
                {
                    if (!cmdDownloadArchiveMetadata(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::UploadArchive)
                {
                    if (!cmdUploadArchive(client, packet, remoteKey))
                        continue;
                }
                else if (packet.type == NetPacket::DeleteArchive)
                {
                    if (!cmdDeleteArchive(client, packet, remoteKey))
                        continue;
                }
                else
                {
                    cerr << "Unknown packet of type "<<(int)packet.type<<" with size "<<packet.data.size()<<" received"<<endl;
                    client.send({NetPacket::Abort});
                }
            }
        }
        catch (const exception& e)
        {
            cout << "Server::handleClient: Caught exception ("<<e.what()<<"), returning"<<endl;
            break;
        }
        catch (...)
        {
            cout << "Server::handleClient: Caught exception, returning"<<endl;
            break;
        }
    }
}
