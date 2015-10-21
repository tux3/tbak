#include "commands.h"
#include "folderdb.h"
#include "nodedb.h"
#include "settings.h"
#include "util/humanreadable.h"
#include "server.h"
#include "net/netsock.h"
#include "net/netaddr.h"
#include "net/net.h"
#include "serialize.h"
#include "compression.h"
#include "filetime.h"
#include "util/pathtools.h"
#include "util/vt100.h"
#include "threadedworker.h"
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
                 "folder status <path> : Query the nodes for this folder's status\n"
                 "folder add-source <path> : Start tracking a local folder\n"
                 "folder add-archive <path> : Start tracking a remote folder\n"
                 "folder remove-source <path> : Stop tracking a source folder\n"
                 "folder remove-archive <path> : Stop tracking an archive folder\n"
                 "folder push <path> : Send the folder to other nodes's archive\n"
                 "folder restore <path> : Download missing files from other node's archives\n"
                 "node showkey : Show our node's public key\n"
                 "node show : Show the list of remote nodes\n"
                 "node add <URL> [<key>] : Add a remote node by hostname, optionally with the provided public key\n"
                 "node remove <URL> : Remove a remote node\n"
                 "node start : Start running as a server node\n"
              << std::flush;
}

void folderShow()
{
    FolderDB fdb(folderDBPath());

    printf("%*s %*s %*s\n",48,"Path",8,"Type",12,"Space used");
    for (const Source& f : fdb.getSources())
    {
        std::string path = f.getPath(), type = "Source",
                size = humanReadableSize(f.getSize());
        printf("%*s ",48, path.c_str());
        printf("%*s ",8, type.c_str());
        printf("%*s \n",12, size.c_str());

    }
    for (const Archive& f : fdb.getArchives())
    {
        std::string path = f.getPathHash().toBase64(), type = "Archive",
                size = humanReadableSize(f.getActualSize());
        printf("%*s ",48, path.c_str());
        printf("%*s ",8, type.c_str());
        printf("%*s \n",12, size.c_str());

    }
}

void folderRemoveSource(const string &path)
{
    FolderDB fdb(folderDBPath());
    fdb.removeSource(normalizePath(path));
}

void folderRemoveArchive(const string &pathHashStr)
{
    FolderDB fdb(folderDBPath());
    fdb.removeArchive(pathHashStr);
}

void folderAddSource(const string &path)
{
    FolderDB fdb(folderDBPath());
    fdb.addSource(normalizePath(path));
}

bool folderAddArchive(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string folderpath{normalizePath(path)};
    PathHash pathHash{folderpath};

    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        NetSock sock;
        try {
            NetSock sockTry(NetAddr{node.getUri()});
            sock = move(sockTry);
        } catch (const runtime_error& e) {
            cout << "Failed to connect to node "<<node.getUri()<<endl;
            continue;
        }
        if (!Net::sendAuth(sock, server))
        {
            cerr << "Couldn't authenticate with that node"<<endl;
            return false;
        }

        NetPacket reply = sock.secureRequest({NetPacket::FolderStats, ::serialize(pathHash)}, server, node.getPk());
        if (reply.type != NetPacket::FolderStats)
        {
            cout << "Node "<<node.getUri()<<" doesn't have this archive"<<endl;
            continue;
        }
        cout << "Found archive on node "<<node.getUri()<<endl;
        fdb.addArchive(pathHash);
        break;
    }
    return true;
}

bool folderPush(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string sourcePath{normalizePath(path)};
    PathHash sourcePathHash{sourcePath};

    // Make a list of local files
    const Source* src = fdb.getSource(sourcePath);
    if (!src)
    {
        cout <<"Source folder "<<sourcePath<<" not found"<<endl;
        return false;
    }
    cout << "Building list of local files..."<<flush;
    vector<SourceFile> lEntries = src->getSourceFiles();
    sort(begin(lEntries), end(lEntries));
    cout << vt100::CLEARLINE() << "Found "<<lEntries.size()<<" local files"<<endl;

    // Push to all the nodes
    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        if (Server::abortall)
            return true;
        NetSock sock;
        try {
            NetSock sockTry(NetAddr{node.getUri()});
            sock = move(sockTry);
        } catch (const runtime_error& e) {
            cout << "Failed to connect to node "<<node.getUri()<<endl;
            continue;
        }

        if (!Net::sendAuth(sock, server))
        {
            cout << "Couldn't authenticate with node "<<node.getUri()<<endl;
            continue;
        }
        cout << "Pushing to node "<<node.getUri()<<endl;

        // Try to get the content list of the folder, create it if necessary
        vector<FileTime> rEntries;
        try {
            rEntries = node.fetchFolderList(sock, server, sourcePathHash);
        } catch (const runtime_error& e) {
            cout<<"Node "<<node.getUri()<<" doesn't have this folder, creating it"<<endl;
            if (!node.createFolder(sock, server, sourcePathHash))
            {
                cout << "Couldn't create the folder on node "+node.getUri()+'\n';
                continue;
            }

            try {
                rEntries = node.fetchFolderList(sock, server, sourcePathHash);
            } catch (const runtime_error& e) {
                cout << "Error: "<<e.what()<<endl;
                continue;
            }
        }

        // Both lists are sorted by hash, we iterate over both at the same time
        // This allows us to find the files to upload and delete in one pass
        cout << "Building diff..."<<flush;
        vector<SourceFile> updiff; // Files we need to upload
        vector<FileTime> deldiff; // Files we need to delete
        if (rEntries.size() > lEntries.size())
            deldiff.reserve(rEntries.size() - lEntries.size());
        else if (lEntries.size() > rEntries.size())
            updiff.reserve(lEntries.size() - rEntries.size());
        sort(begin(rEntries), end(rEntries));
        {
            auto lit = lEntries.begin(), lend = lEntries.end();
            auto rit = rEntries.begin(), rend = rEntries.end();
            while (lit != lend && rit != rend)
            {
                // If we don't have this remote file
                if (rit->hash < lit->getPathHash())
                {
                    deldiff.push_back(*rit);
                    ++rit;
                }
                // If the remote doesn't have this file
                else if (lit->getPathHash() < rit->hash)
                {
                    updiff.push_back(*lit);
                    ++lit;
                }
                // If we both have this file
                else
                {
                    if (rit->mtime != lit->getAttrs().mtime)
                        updiff.push_back(*lit);
                    ++lit;
                    ++rit;
                }
            }
            updiff.insert(updiff.end(), lit, lend);
            deldiff.insert(deldiff.end(), rit, rend);
        }

        cout <<vt100::CLEARLINE()<<"Need to upload "<<updiff.size()
            <<" files and delete "<<deldiff.size()<<" remote files"<<endl;

        // If the upload might be interrupted, it's more useful to not upload in a random-ish order
        sort(begin(updiff), end(updiff), [](const SourceFile& f1, const SourceFile& f2)
        {
            return f1.getPath()<f2.getPath();
        });

        ThreadedWorker worker(sock, server, node);
        worker.uploadFiles(sourcePathHash, updiff);
        worker.deleteFiles(sourcePathHash, deldiff);
    }
    return true;
}

void folderStatus(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    printf("%*s %*s\n",24,"URI",12,"Size");
    string folderPath{normalizePath(path)};
    PathHash folderPathHash{folderPath};

    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        if (Server::abortall)
            return;
        NetSock sock;
        try {
            NetSock sockTry(NetAddr{node.getUri()});
            sock = move(sockTry);
        } catch (const runtime_error& e) {
            cout << "Failed to connect to node "<<node.getUri()<<endl;
            continue;
        }
        if (!Net::sendAuth(sock, server))
        {
            printf("%*s (couldn't authenticate with node)\n",24,node.getUri().c_str());
            continue;
        }

        NetPacket request{NetPacket::FolderStats, ::serialize(folderPathHash)};
        Crypto::encryptPacket(request, server, node.getPk());
        sock.send(request);
        NetPacket reply = sock.recvPacket();
        Crypto::decryptPacket(reply, server, node.getPk());
        if (reply.type != NetPacket::FolderStats)
        {
            printf("%*s ???\n",24,node.getUri().c_str());
            continue;
        }
        auto it = reply.data.cbegin();
        uint64_t size = ::deserializeConsume<uint64_t>(it);
        printf("%*s %*s\n",24,node.getUri().c_str(),
                                    12, humanReadableSize(size).c_str());
    }
}

bool folderRestore(const string &path)
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    string sourcePath{normalizePath(path)};
    PathHash sourcePathHash{sourcePath};

    // Make a list of local files
    Source* src = fdb.getSource(sourcePath);
    if (!src)
    {
        cout <<"Source folder "<<sourcePath<<" not found"<<endl;
        return false;
    }
    cout << "Building list of local files..."<<flush;
    vector<SourceFile> lEntries = src->getSourceFiles();
    sort(begin(lEntries), end(lEntries));
    cout << vt100::CLEARLINE() << "Found "<<lEntries.size()<<" local files"<<endl;

    // Push to all the nodes
    Server server(serverConfigPath(), ndb, fdb);
    const vector<Node>& nodes = ndb.getNodes();
    for (const Node& node : nodes)
    {
        if (Server::abortall)
            return true;
        NetSock sock;
        try {
            NetSock sockTry(NetAddr{node.getUri()});
            sock = move(sockTry);
        } catch (const runtime_error& e) {
            cout << "Failed to connect to node "<<node.getUri()<<endl;
            continue;
        }
        if (!Net::sendAuth(sock, server))
        {
            cout << "Couldn't authenticate with node "<<node.getUri()<<endl;
            continue;
        }
        cout << "Restoring from node "<<node.getUri()<<endl;

        // Try to get the content list of the folder, create it if necessary
        vector<FileTime> rEntries;
        try {
            rEntries = node.fetchFolderList(sock, server, sourcePathHash);
        } catch (const runtime_error& e) {
            cout<<"Node "<<node.getUri()<<" doesn't have this folder, skipping it"<<endl;
            continue;
        }

        // Both lists are sorted by hash, we iterate over both at the same time
        // This allows us to find the files to download in one pass
        cout << "Building diff..."<<flush;
        vector<FileTime> downdiff; // Files we need to download
        if (rEntries.size() > lEntries.size())
            downdiff.reserve(rEntries.size() - lEntries.size());
        sort(begin(rEntries), end(rEntries));
        {
            auto lit = lEntries.begin(), lend = lEntries.end();
            auto rit = rEntries.begin(), rend = rEntries.end();
            while (lit != lend && rit != rend)
            {
                // If we don't have this remote file
                if (rit->hash < lit->getPathHash())
                {
                    downdiff.push_back(*rit);
                    ++rit;
                }
                // If the remote doesn't have this file
                else if (lit->getPathHash() < rit->hash)
                {
                    ++lit;
                }
                // If we both have this file
                else
                {
                    if (rit->mtime > lit->getAttrs().mtime)
                        downdiff.push_back(*rit);
                    ++lit;
                    ++rit;
                }
            }
            downdiff.insert(downdiff.end(), rit, rend);
        }

        cout <<vt100::CLEARLINE()<<"Need to download "<<downdiff.size()<<" files"<<endl;

        for (const FileTime& file : downdiff)
        {
            cout << "Downloading encrypted file metadata... "<<flush;
            uint64_t fileSize;
            string filePath;
            vector<char> meta;
            {
                meta = node.downloadFileMetadata(sock, server, sourcePathHash, file.hash);
                auto it = meta.cend()-sizeof(uint64_t);
                fileSize = ::deserializeConsume<uint64_t>(it);
                meta.erase(meta.cend()-sizeof(uint64_t), meta.cend());
                Crypto::decrypt(meta, server, server.getPublicKey());
                it = meta.cbegin();
                filePath = ArchiveFile::deserializePath(it);
            }

            cout << vt100::CLEARLINE() << "Downloading file "<<filePath
                 <<" ("<<humanReadableSize(fileSize)<<")..."<<endl;
            vector<char> data = node.downloadFile(sock, server, sourcePathHash, file.hash);
            uint64_t mtime;
            {
                auto it = data.cbegin();
                mtime = ::deserializeConsume<uint64_t>(it);
                size_t msize = ::dataToVUint(it);
                data.erase(data.begin(), it+msize);
            }
            Crypto::decrypt(data, server, server.getPublicKey());
            data = Compression::inflate(data);

            src->restoreFile(meta, mtime, data);
        }
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

void nodeAdd(const string &uri, const string &pk)
{
    NodeDB ndb(nodeDBPath());
    ndb.addNode(uri, pk);
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
    return r==0;
}

void nodeShowkey()
{
    FolderDB fdb(folderDBPath());
    NodeDB ndb(nodeDBPath());
    Server server(serverConfigPath(), ndb, fdb);
    auto pk = server.getPublicKey();
    cout << "Our public key is " << Crypto::keyToString(pk)<<endl;
}

}
