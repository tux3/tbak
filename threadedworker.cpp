#include "threadedworker.h"
#include "node.h"
#include "net/netsock.h"
#include "net/netpacket.h"
#include "util/humanreadable.h"
#include "serialize.h"
#include "util/vt100.h"
#include "compression.h"
#include "server.h"
#include <iostream>
#include <queue>

using namespace std;
using namespace vt100;

ThreadedWorker::ThreadedWorker(NetSock &sock, Server &server, const Node &remote)
    : sock{sock}, server{server}, node{remote}
{
}

void ThreadedWorker::deleteFiles(PathHash folderHash, const vector<FileTime>& deldiff)
{
    queue<const FileTime*> netQueue;
    int total = deldiff.size(), cur = 1;
    auto progress = [&](){return "["+to_string(cur)+'/'+to_string(total)+"] ";};
    auto fit = deldiff.cbegin();

    cout << MOVEUP(1);

    while (fit != deldiff.cend() || !netQueue.empty())
    {
        if (sock.isPacketAvailable())
        {
            NetPacket reply = sock.recvPacket();
            int queueSize = netQueue.size();
            cout << MOVEUP(queueSize-1) << CLEARLINE();
            const FileTime* f = netQueue.front();
            if (reply.type == NetPacket::DeleteArchive)
                cout << "Deleted old remote file (hash "<<f->hash.toBase64()<<')';
            else
                cout << STYLE_ERROR() << "Failed to delete old remote file with hash "
                     <<f->hash.toBase64() << STYLE_RESET();
            cout << MOVEDOWN(queueSize-1) << flush;
            netQueue.pop();
        }

        if (netQueue.size() < netQueueSize && fit != deldiff.cend())
        {
            netQueue.push(&*fit);
            cout << STYLE_ACTIVE();
            cout << '\n' << progress() << "Deleting old remote file (hash "<<fit->hash.toBase64() << ")..."
                 << STYLE_RESET() << flush;
            vector<char> data;
            serializeAppend(data, folderHash);
            serializeAppend(data, fit->hash);
            sock.sendEncrypted(NetPacket{NetPacket::DeleteArchive, move(data)}, server, node.getPk());
            fit++;
            cur++;
        }
    }
    cout << endl;
}

void ThreadedWorker::uploadFiles(PathHash folderHash, const std::vector<SourceFile> &updiff)
{
    queue<const SourceFile*> netQueue;
    int total = updiff.size(), cur = 1;
    auto progress = [&](){return "["+to_string(cur)+'/'+to_string(total)+"] ";};
    auto fit = updiff.cbegin();

    cout << MOVEUP(1);

    while (fit != updiff.cend() || !netQueue.empty())
    {
        if (sock.isPacketAvailable())
        {
            NetPacket reply = sock.recvPacket();
            int queueSize = netQueue.size();
            cout << MOVEUP(queueSize-1) << CLEARLINE();
            const SourceFile* f = netQueue.front();
            if (reply.type == NetPacket::UploadArchive)
                cout << "Uploaded "<<f->getPath()<<" ("
                     <<humanReadableSize(f->getRawSize())<<')';
            else
                cout << STYLE_ERROR() << "Failed to upload "<<f->getPath()<<" ("
                     <<humanReadableSize(f->getRawSize())<<')'<< STYLE_RESET();
            cout << MOVEDOWN(queueSize-1) << flush;
            netQueue.pop();
        }

        if (netQueue.size() < netQueueSize && fit != updiff.cend())
        {
            netQueue.push(&*fit);
            cout << STYLE_ACTIVE();
            cout << '\n' << progress() << "Uploading "<<fit->getPath()<<" ("
                 <<humanReadableSize(fit->getRawSize())<<')'<< STYLE_RESET() << flush;
            vector<char> data;
            serializeAppend(data, folderHash);
            serializeAppend(data, fit->getPathHash());
            serializeAppend(data, fit->getAttrs().mtime);
            {
                // Encrypt the metadata and contents separately, so we can later download the metadata only
                vector<char> fileData;
                vector<char> meta = fit->serializeMetadata();
                Crypto::encrypt(meta, server, server.getPublicKey());
                vectorAppend(fileData, vuintToData(meta.size()));
                vectorAppend(fileData, move(meta));
                vector<char> contents = Compression::deflate(fit->readAll());
                Crypto::encrypt(contents, server, server.getPublicKey());
                vectorAppend(fileData, move(contents));
                vectorAppend(data, move(fileData));
            }
            sock.sendEncrypted(NetPacket{NetPacket::UploadArchive, move(data)}, server, node.getPk());
            fit++;
            cur++;
        }
    }
    cout << endl;
}

