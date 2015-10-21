#ifndef THREADEDWORKER_H
#define THREADEDWORKER_H

#include "filetime.h"
#include "pathhash.h"
#include "sourcefile.h"
#include <vector>

class NetSock;
class Server;
class Node;

class ThreadedWorker
{
public:
    ThreadedWorker(NetSock& sock, Server& server, const Node& remote);
    void deleteFiles(PathHash folderHash, const std::vector<FileTime>& deldiff);
    void uploadFiles(PathHash folderHash, const std::vector<SourceFile>& updiff);

public:
    // Limits
    static constexpr int maxNetQueueSize = 10,
                        maxZipQueueSize = 4096, maxZipDataSize = 50*1024*1024;
private:
    NetSock& sock;
    Server& server;
    const Node& node;
};

#endif // THREADEDWORKER_H
