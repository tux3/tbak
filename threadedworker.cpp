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
#include <thread>
#include <boost/lockfree/spsc_queue.hpp>

using namespace std;
using namespace vt100;
using namespace boost::lockfree;

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
        if (sock.isShutdown() || server.abortall)
        {
            int queueSize = netQueue.size();
            cout << MOVEUP(queueSize) << STYLE_ERROR();
            while (queueSize--)
                cout << CLEARLINE() << "Operation aborted." << MOVEDOWN(1);
            cout << STYLE_RESET();
            return;
        }

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

        if (netQueue.size() < maxNetQueueSize && fit != deldiff.cend())
        {
            netQueue.push(&*fit);
            cout << STYLE_ACTIVE();
            cout << '\n' << progress() << "Deleting old remote file (hash "<<fit->hash.toBase64() << ")..."
                 << STYLE_RESET() << flush;
            node.deleteFileAsync(sock, server, folderHash, fit->hash);
            fit++;
            cur++;
        }
    }
    cout << endl;
}

/// Compresses, encrypts, and serializes files in the background
/// Takes a billion arguments because if it was a member function, we'd have to include
/// boost lockfree headers in our public header, ruining compile times...
static void zipFiles(spsc_queue<vector<char>*, capacity<ThreadedWorker::maxZipQueueSize>>& zipQueue,
                     const std::vector<SourceFile> &updiff, atomic_int& zippedDataSize,
                     const atomic_bool& stopNow, const PathHash& folderHash,
                     const Server& s)
{
    auto fit = updiff.cbegin();
    while (fit != updiff.cend() && !stopNow)
    {
        if (zippedDataSize > ThreadedWorker::maxZipDataSize
                || zipQueue.read_available() >= ThreadedWorker::maxZipQueueSize)
        {
            this_thread::sleep_for(50ms);
            continue;
        }

        // We build our serialzed data here, the consumer thread will delete it
        vector<char>& data = *new vector<char>();
        serializeAppend(data, folderHash);
        serializeAppend(data, fit->getPathHash());
        serializeAppend(data, fit->getAttrs().mtime);
        {
            // Encrypt the metadata and contents separately, so we can later download the metadata only
            vector<char> fileData;
            vector<char> meta = fit->serializeMetadata();
            Crypto::encrypt(meta, s, s.getPublicKey());
            vectorAppend(fileData, vuintToData(meta.size()));
            vectorAppend(fileData, move(meta));
            vector<char> contents = Compression::deflate(fit->readAll());
            Crypto::encrypt(contents, s, s.getPublicKey());
            vectorAppend(fileData, move(contents));
            vectorAppend(data, move(fileData));
        }
        zippedDataSize += data.size();
        zipQueue.push(&data);
        ++fit;
    }
}

void ThreadedWorker::uploadFiles(PathHash folderHash, const std::vector<SourceFile> &updiff)
{
    queue<const SourceFile*> netQueue;
    int total = updiff.size(), cur = 1;
    auto progress = [&](){return "["+to_string(cur)+'/'+to_string(total)+"] ";};
    auto fit = updiff.cbegin();

    spsc_queue<vector<char>*, capacity<maxZipQueueSize>> zipQueue;
    atomic_int zippedDataSize{0};
    atomic_bool stopNow{false};
    thread zipThread(zipFiles, ref(zipQueue), ref(updiff), ref(zippedDataSize),
                     ref(stopNow), ref(folderHash), ref(server));

    cout << MOVEUP(1);
    while (fit != updiff.cend() || !netQueue.empty())
    {
        if (sock.isShutdown() || server.abortall)
        {
            int queueSize = netQueue.size();
            cout << MOVEUP(queueSize) << STYLE_ERROR();
            while (queueSize--)
                cout << CLEARLINE() << "Operation aborted." << MOVEDOWN(1);
            cout << STYLE_RESET();

            stopNow = true;
            zipThread.join();
            return;
        }

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

        if (netQueue.size() < maxNetQueueSize && fit != updiff.cend()
                && zipQueue.read_available())
        {
            netQueue.push(&*fit);
            cout << STYLE_ACTIVE();
            cout << '\n' << progress() << "Uploading "<<fit->getPath()<<" ("
                 <<humanReadableSize(fit->getRawSize())<<')'<< STYLE_RESET() << flush;
            vector<char>* serializedData = nullptr;
            zipQueue.pop(&serializedData, 1);
            zippedDataSize -= serializedData->size();
            sock.sendEncrypted({NetPacket::UploadArchive, *serializedData}, server, node.getPk());
            delete serializedData;
            fit++;
            cur++;
        }
    }
    cout << endl;
    zipThread.join();
}

