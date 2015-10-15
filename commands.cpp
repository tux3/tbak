#include "commands.h"
#include "folderdb.h"
#include "nodedb.h"
#include "settings.h"
#include "humanReadable.h"
#include "server.h"
#include "netsock.h"
#include "netaddr.h"
#include "net.h"
#include "serialize.h"
#include "compression.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <cassert>

using namespace std;

namespace cmd
{

void help()
{
    std::cout << "Usage: tbak <command> [<arg(s)>]\n"
                 "Commands:\n"
                 "folder show : Show the list of tracked folders\n"
                 "folder status : Query the nodes for this folder's status\n"
                 "folder add-source : Start tracking a local folder\n"
                 "folder add-archive : Start tracking a remote folder\n"
                 "folder remove-source : Stop tracking a source folder\n"
                 "folder remove-archive : Stop tracking an archive folder\n"
                 "folder push : Ask other nodes to track this folder\n"
                 "folder update : Update the files database manually\n"
                 "folder sync : Update and synchronize this folder with other nodes\n"
                 "folder restore : Overwrite this source folder with the node's archives\n"
                 "node show : Show the list of storage nodes\n"
                 "node add : Add a storage node\n"
                 "node remove : Remove a storage node\n"
                 "node start : Start running as a server\n"
              << std::flush;
}

void folderShow()
{
    FolderDB fdb(folderDBPath());

    printf("%*s %*s %*s\n",48,"Path",8,"Type",12,"Space used");
    for (const Folder& f : fdb.getFolders())
    {
        std::string path = f.getPath(), type = f.getTypeString(),
                size = humanReadableSize(f.getActualSize());
        printf("%*s ",48, path.c_str());
        printf("%*s ",8, type.c_str());
        printf("%*s \n",12, size.c_str());

    }
}

void folderRemoveSource(const string &path)
{
    FolderDB fdb(folderDBPath());
    fdb.removeFolder(Folder::normalizePath(path), false);
}

void folderRemoveArchive(const string &path)
{
    FolderDB fdb(folderDBPath());
    fdb.removeFolder(Folder::normalizePath(path), true);
}

void folderAddSource(const string &path)
{
    FolderDB fdb(folderDBPath());
    fdb.addFolder(Folder::normalizePath(path));
}

bool folderAddArchive(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string folderpath{Folder::normalizePath(path)};

    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        Server server(serverConfigPath(), ndb, fdb);
        NetSock sock(NetAddr{node.getUri()});
        if (!Net::sendAuth(sock, server))
        {
            cerr << "Couldn't authenticate with that node"<<endl;
            return false;
        }

        NetPacket request{NetPacketType::FolderStats, ::serialize(folderpath)};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacketType::FolderStats)
        {
            cout << "Node "<<node.getUri()<<" replied with "<<(int)reply.type<<endl;
            continue;
        }
        cout << "Found folder on node "<<node.getUri()<<endl;
        Folder f{reply.data};
        f.setType(FolderType::Archive);
        f.open(true);
        f.close();
        fdb.addFolder(f);
        break;
    }
    return true;
}

bool folderPush(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string folderpath{Folder::normalizePath(path)};

    Folder* lfolder=fdb.getFolder(folderpath);
    if (lfolder)
    {
        lfolder->open(true);
        lfolder->close();
    }
    else
    {
        cout <<"Folder not found"<<endl;
        return false;
    }

    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        Server server(serverConfigPath(), ndb, fdb);
        NetSock sock(NetAddr{node.getUri()});
        if (!Net::sendAuth(sock, server))
        {
            cout << "Couldn't authenticate with node "<<node.getUri()<<endl;
            continue;
        }

        NetPacket request{NetPacketType::FolderPush, lfolder->serialize()};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacketType::FolderPush)
        {
            cout << "Couldn't push the folder to node "<<node.getUri()<<endl;
            continue;
        }
    }
    return true;
}

void folderUpdate(const string &path)
{
    FolderDB fdb(folderDBPath());
    string folderpath{Folder::normalizePath(path)};

    Folder* f = fdb.getFolder(folderpath);
    f->open(true);
    f->close();
}

void folderStatus(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    printf("%*s %*s %*s %*s\n",24,"URI",8,"Type",12,"Raw size",12,"Actual size");
    string folderpath{Folder::normalizePath(path)};
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        Server server(serverConfigPath(), ndb, fdb);
        NetSock sock;
        if (!sock.connect(node.getUri()))
        {
            printf("%*s (couldn't connect to node)\n",24,node.getUri().c_str());
            continue;
        }
        if (!Net::sendAuth(sock, server))
        {
            printf("%*s (couldn't authenticate with node)\n",24,node.getUri().c_str());
            continue;
        }

        NetPacket request{NetPacketType::FolderStats, ::serialize(folderpath)};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacketType::FolderStats)
        {
            printf("%*s ???\n",24,node.getUri().c_str());
            continue;
        }
        Folder f{reply.data};
        printf("%*s %*s %*s %*s\n",24,node.getUri().c_str(), 8, f.getTypeString().c_str(),
                                    12, humanReadableSize(f.getRawSize()).c_str(),
                                    12, humanReadableSize(f.getActualSize()).c_str());
    }
}

bool folderSync(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string folderpath{Folder::normalizePath(path)};

    struct entry {
        string path;
        uint64_t mtime;
        bool operator<(const entry& other){return path<other.path;}
    };
    vector<entry> lEntries;

    Folder* lfolder=fdb.getFolder(folderpath);
    if (lfolder)
    {
        lfolder->open(true);
        for (const File& file : lfolder->getFiles())
        {
            entry e;
            e.path = file.path;
            e.mtime = file.attrs.mtime;
            lEntries.push_back(e);
        }
        lfolder->close();
    }
    else
    {
        cout <<"Folder not found"<<endl;
        return false;
    }

    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        cout << "Syncing with "<<node.getUri()<<"..."<<endl;
        Server server(serverConfigPath(), ndb, fdb);
        NetSock sock;
        if (!sock.connect(node.getUri()))
        {
            cout << "Couldn't connect to node "<<node.getUri()<<endl;
            continue;
        }
        if (!Net::sendAuth(sock, server))
        {
            cout << "Couldn't authenticate with node "<<node.getUri()<<endl;
            continue;
        }

        // Get status of remote folder
        unique_ptr<Folder> rfolder;
        {
            NetPacket request{NetPacketType::FolderStats, ::serialize(folderpath)};
            Crypto::encryptPacket(request, server, node.getPk());
            sock.send(request);
            NetPacket reply = sock.recvPacket();
            Crypto::decryptPacket(reply, server, node.getPk());
            if (reply.type != NetPacketType::FolderStats)
            {
                cout<<"Unable to get folder stats of remote node "<<node.getUri()<<endl;
                continue;
            }
            rfolder = move(unique_ptr<Folder>(new Folder{reply.data}));
        }

        // Send a FolderSourceReload if needed
        if (rfolder->getType() == FolderType::Source)
        {
            NetPacket request{NetPacketType::FolderSourceReload, ::serialize(folderpath)};
            Crypto::encryptPacket(request, server, node.getPk());
            sock.send(request);
            NetPacket reply = sock.recvPacket();
            if (reply.type != NetPacketType::FolderSourceReload)
                cout<<"Remote node couldn't reload from source, continuing anyway..."<<endl;
        }

        // Get list of files and their last access times
        NetPacket request{NetPacketType::FolderTimeList, ::serialize(folderpath)};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacketType::FolderTimeList)
        {
            cout << "Failed to sync with node "<<node.getUri()<<endl;
            continue;
        }

        vector<char> rFileTimes = Compression::inflate(reply.data);
        reply.data.clear();
        reply.data.shrink_to_fit();
        vector<entry> rEntries;
        auto it = rFileTimes.cbegin();
        while (it != rFileTimes.cend())
        {
            entry e;
            e.path = ::deserializeConsume<string>(it);
            e.mtime = ::deserializeConsume<uint64_t>(it);
            rEntries.push_back(e);
        }
        rFileTimes.clear();
        rFileTimes.shrink_to_fit();

        cout << "Got "<<rEntries.size()<<" remote entries"<<endl;
        cout << "Got "<<lEntries.size()<<" local entries"<<endl;

        // Update our local copy (if it's an archive, because sources are read-only)
        if (lfolder->getType() == FolderType::Archive)
        {
            lfolder->open();

            // Remove files if the remote source removed them
            if (rfolder->getType() == FolderType::Source)
            {
                vector<entry> diff;
                set_difference(begin(lEntries), end(lEntries), begin(rEntries), end(rEntries),back_inserter(diff));
                cout << "Need to remove "<<diff.size()<<" files"<<endl;
            }

            // Download files that are newer on the remote
            {
                vector<entry> diff;
                copy_if(begin(rEntries), end(rEntries), back_inserter(diff), [&lEntries](const entry& re)
                {
                    auto it = find_if(begin(lEntries), end(lEntries), [re](const entry& e)
                    {
                        return e.path == re.path;
                    });
                    if (it == end(lEntries))
                        return true;
                    else
                        return it->mtime < re.mtime;
                });
                cout << "Need to download "<<diff.size()<<" files"<<endl;
                int inFlight = 0;
                vector<char> folderPathData = ::serialize(folderpath);
                for (entry e : diff)
                {
                    if (server.abortall)
                        return true;

                    cout << "Downloading "<<e.path<<"...\n";
                    NetPacket request{NetPacketType::DownloadArchiveFile, folderPathData};
                    vectorAppend(request.data, ::serialize(e.path));
                    Crypto::encryptPacket(request, server, node.getPk());
                    try {
                    sock.send(request);
                    } catch (...) {
                        cout << "Download failed, couldn't send request\n";
                        continue;
                    }

                    inFlight++;

                    // Check reply
                    while (inFlight && (inFlight >= maxPacketsInFlight
                                        || sock.isPacketAvailable()))
                    {
                        inFlight--;
                        NetPacket reply;
                        try {
                            reply = sock.recvPacket();
                            Crypto::decryptPacket(reply, server, node.getPk());
                        } catch (runtime_error e) {
                            if (server.abortall)
                                return true;
                            cout <<"Caught unexpected exception ("<<e.what()<<"), quitting"<<endl;
                            return false;
                        } catch (...) {
                            cout << "Caught an unknown exception, quitting"<<endl;
                            return false;
                        }

                        if (reply.type != NetPacketType::DownloadArchiveFile)
                        {
                            cout << "Download failed\n";
                            continue;
                        }
                        lfolder->writeArchiveFile(reply.data, server, node.getPk());
                    }
                }
            }

            lfolder->close();
        }

        // Remove remote files that don't exist on our source
        if (lfolder->getType() == FolderType::Source
            && rfolder->getType() == FolderType::Archive)
        {
            /// TODO: Sort this by path and use set_difference
            vector<entry> diff;
            copy_if(begin(rEntries), end(rEntries), back_inserter(diff), [&lEntries](const entry& re)
            {
                auto it = find_if(begin(lEntries), end(lEntries), [re](const entry& e)
                {
                    return e.path == re.path;
                });
                return it == end(lEntries);
            });

            cout << "Need to remove "<<diff.size()<<" remote files"<<endl;
            vector<char> fdata = ::serialize(lfolder->getPath());
            int inFlight = 0;
            for (entry e : diff)
            {
                if (server.abortall)
                    return true;
                cout << "Removing "<<e.path<<"...\n";

                NetPacket request{NetPacketType::DeleteArchiveFile, fdata};
                vectorAppend(request.data, ::serialize(e.path));
                Crypto::encryptPacket(request, server, node.getPk());
                try {
                sock.send(request);
                } catch (...) {
                    cout << "Removal failed, couldn't send request\n";
                    continue;
                }

                inFlight++;

                // Check reply
                while (inFlight && (inFlight >= maxPacketsInFlight
                                    || sock.isPacketAvailable()))
                {
                    inFlight--;
                    NetPacket reply;
                    try {
                        reply = sock.recvPacket();
                        Crypto::decryptPacket(reply, server, node.getPk());
                    } catch (runtime_error e) {
                        if (server.abortall)
                            return true;
                        cout <<"Caught unexpected exception ("<<e.what()<<"), quitting"<<endl;
                        return false;
                    } catch (...) {
                        cout << "Caught an unknown exception, quitting"<<endl;
                        return false;
                    }

                    if (reply.type != NetPacketType::DeleteArchiveFile)
                    {
                        cout << "Removal failed\n";
                        continue;
                    }
                }
            }
        }

        // Upload files that are newer on our side
        if (rfolder->getType() == FolderType::Archive)
        {
            vector<entry> diff;
            copy_if(begin(lEntries), end(lEntries), back_inserter(diff), [&rEntries](const entry& le)
            {
                auto it = find_if(begin(rEntries), end(rEntries), [le](const entry& e)
                {
                    return e.path == le.path;
                });
                if (it == end(rEntries))
                    return true;
                else
                    return it->mtime < le.mtime;
            });
            cout << "Need to upload "<<diff.size()<<" files"<<endl;
            int inFlight = 0;
            for (entry e : diff)
            {
                if (server.abortall)
                    return true;
                cout << "Uploading "<<e.path<<"...\n";

                // Find file
                const std::vector<File>& files = lfolder->getFiles();
                bool found = false;
                const File* file = nullptr;
                for (const File& f : files)
                {
                    if (f.path == e.path)
                    {
                        file = &f;
                        found = true;
                        break;
                    }
                }
                assert(found);
                vector<char> fdata = ::serialize(lfolder->getPath());

                // Compress and encrypt
                {
                    File archived = *file;
                    vector<char> content = file->readAll();
                    content = Compression::deflate(content);
                    Crypto::encrypt(content, server, node.getPk());
                    archived.actualSize = archived.metadataSize() + content.size();
                    vectorAppend(fdata, archived.serialize());
                    copy(move_iterator<vector<char>::iterator>(begin(content)),
                         move_iterator<vector<char>::iterator>(end(content)),
                         back_inserter(fdata));
                }

                // Send result
                NetPacket request{NetPacketType::UploadArchiveFile, fdata};
                Crypto::encryptPacket(request, server, node.getPk());
                sock.send(request);
                inFlight++;

                // Check reply
                while (inFlight && (inFlight >= maxPacketsInFlight
                                    || sock.isPacketAvailable()))
                {
                    inFlight--;
                    NetPacket reply = sock.recvPacket();
                    Crypto::decryptPacket(reply, server, node.getPk());
                    if (reply.type != NetPacketType::UploadArchiveFile)
                    {
                        cout << "Upload failed\n";
                        continue;
                    }
                }
            }
        }
    }
    return true;
}

bool folderRestore(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string folderpath{Folder::normalizePath(path)};

    struct entry {
        string path;
        uint64_t mtime;
        bool operator<(const entry& other){return path<other.path;}
    };
    vector<entry> lEntries;
    Folder* lfolder=fdb.getFolder(folderpath);
    if (lfolder && lfolder->getType() == FolderType::Source)
    {
        lfolder->open(true);
        for (const File& file : lfolder->getFiles())
        {
            entry e;
            e.path = file.path;
            e.mtime = file.attrs.mtime;
            lEntries.push_back(e);
        }
        lfolder->close();
    }
    else
    {
        cerr << "Local source folder to restore not found"<<endl;
        return false;
    }



    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        cout << "Restoring from "<<node.getUri()<<"..."<<endl;
        Server server(serverConfigPath(), ndb, fdb);
        NetSock sock(NetAddr{node.getUri()});
        if (!Net::sendAuth(sock, server))
        {
            cerr << "Couldn't authenticate with that node"<<endl;
            return false;
        }

        // Get status of remote folder
        unique_ptr<Folder> rfolder;
        {
            NetPacket request{NetPacketType::FolderStats, ::serialize(folderpath)};
            Crypto::encryptPacket(request, server, node.getPk());
            sock.send(request);
            NetPacket reply = sock.recvPacket();
            Crypto::decryptPacket(reply, server, node.getPk());
            if (reply.type != NetPacketType::FolderStats)
            {
                cout<<"Unable to get folder stats of remote node "<<node.getUri()<<endl;
                continue;
            }
            rfolder = move(unique_ptr<Folder>(new Folder{reply.data}));
        }

        // Get list of files and their last access times
        NetPacket request{NetPacketType::FolderTimeList, ::serialize(folderpath)};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacketType::FolderTimeList)
        {
            cout << "Failed to restore from node "<<node.getUri()<<endl;
            continue;
        }

        vector<char> rFileTimes = Compression::inflate(reply.data);
        reply.data.clear();
        reply.data.shrink_to_fit();
        vector<entry> rEntries;
        auto it = rFileTimes.cbegin();
        while (it != rFileTimes.cend())
        {
            entry e;
            e.path = ::deserializeConsume<string>(it);
            e.mtime = ::deserializeConsume<uint64_t>(it);
            rEntries.push_back(e);
        }
        rFileTimes.clear();
        rFileTimes.shrink_to_fit();

        cout << "Got "<<rEntries.size()<<" remote entries"<<endl;
        cout << "Got "<<lEntries.size()<<" local entries"<<endl;

        // Restore our local source with the remote archive
        lfolder->open();
        {
            vector<entry> diff;
            copy_if(begin(rEntries), end(rEntries), back_inserter(diff), [&lEntries](const entry& re)
            {
                auto it = find_if(begin(lEntries), end(lEntries), [re](const entry& e)
                {
                    return e.path == re.path;
                });
                if (it == end(lEntries))
                    return true;
                else
                    return it->mtime < re.mtime;
            });
            cout << "Need to download "<<diff.size()<<" files"<<endl;

            vector<char> folderPathData = ::serialize(folderpath);
            for (entry e : diff)
            {
                if (server.abortall)
                    return true;

                cout << "Downloading "<<e.path<<"...\n";
                NetPacket request{NetPacketType::DownloadArchiveFile, folderPathData};
                vectorAppend(request.data, ::serialize(e.path));
                Crypto::encryptPacket(request, server, node.getPk());
                sock.send(request);
                NetPacket reply = sock.recvPacket();
                Crypto::decryptPacket(reply, server, node.getPk());
                if (reply.type != NetPacketType::DownloadArchiveFile)
                {
                    cout << "Download failed\n";
                    continue;
                }
                lfolder->writeArchiveFile(reply.data, server, node.getPk());
            }
        }
        lfolder->close();
    }
    return true;
}

void nodeShow()
{
    NodeDB ndb(nodeDBPath());
    printf("%*s %s\n",65,"Public key","URI");
    for (const Node& n : ndb.getNodes())
    {
        printf("%*s ", 65, n.getPkString().c_str());
        printf("%s\n", n.getUri().c_str());
    }
}

void nodeAdd(const string &uri)
{
    NodeDB ndb(nodeDBPath());
    ndb.addNode(uri);
}

void nodeRemove(const string &uri)
{
    NodeDB ndb(nodeDBPath());
    ndb.removeNode(uri);
}

bool nodeStart()
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    Server server(serverConfigPath(), ndb, fdb);
    int r = server.exec();
    cout << "Server exiting with status "<<r<<endl;
    fdb.save();
    ndb.save();
    return r==0;
}

}
