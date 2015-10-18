#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <memory>
#include <algorithm>
#include <thread>
#include <cassert>
#include "nodedb.h"
#include "folderdb.h"
#include "settings.h"
#include "util/humanreadable.h"
#include "server.h"
#include "net/netpacket.h"
#include "net/netaddr.h"
#include "net/net.h"
#include "serialize.h"
#include "compression.h"
#include "sighandlers.h"
#include "commands.h"
#include "util/vt100.h"

using namespace std;

void checkDataDir()
{
    struct stat buf;
    if (stat(dataPath().c_str(), &buf) < 0)
    {
        mkdir(dataPath().c_str(), S_IRWXU | S_IRGRP | S_IWGRP);
        mkdir((dataPath()+"/archive/").c_str(), S_IRWXU | S_IRGRP | S_IWGRP);
    }
    else if (!S_ISDIR(buf.st_mode))
    {
        cerr << "WARNING: checkDataDir: Can't create data dir, file exists : "<<dataPath()<<endl;
    }
}

int main(int argc, char* argv[])
{
    using namespace cmd;

    if (argc<2)
    {
        help();
        return EXIT_FAILURE;
    }

    string command{argv[1]};
    if (command == "help")
    {
        help();
        return EXIT_SUCCESS;
    }

    setupSigHandlers();
    checkDataDir();
    sodium_init();

    if (argc<3)
    {
        help();
        return EXIT_FAILURE;
    }

    string subcommand{argv[2]};
    if (command == "folder")
    {
        if (subcommand == "show")
        {
            folderShow();
        }
        else if (argc < 4)
        {
            help();
            return EXIT_FAILURE;
        }
        else if (subcommand == "remove-source")
        {
            folderRemoveSource(argv[3]);
        }
        else if (subcommand == "remove-archive")
        {
            folderRemoveArchive(argv[3]);
        }
        else if (subcommand == "add-source")
        {
            folderAddSource(argv[3]);
        }
        else if (subcommand == "add-archive")
        {
            if (!folderAddArchive(argv[3]))
                return EXIT_FAILURE;
        }
        else if (subcommand == "push")
        {
            if (!folderPush(argv[3]))
                return EXIT_FAILURE;
        }
        else if (subcommand == "status")
        {
            folderStatus(argv[3]);
        }
        else if (subcommand == "restore")
        {
            folderRestore(argv[3]);
        }
        else
        {
            cout << "Not implemented\n";
        }
    }
    else if (command == "node")
    {
        if (subcommand == "show")
        {
            nodeShow();
        }
        else if (subcommand == "showkey")
        {
            nodeShowkey();
        }
        else if (subcommand == "start")
        {
            if (!nodeStart())
                return EXIT_FAILURE;
        }
        else if (argc < 4)
        {
            help();
            return EXIT_FAILURE;
        }
        else if (subcommand == "add")
        {
            if (argc >= 5)
                nodeAdd(argv[3], argv[4]);
            else
                nodeAdd(argv[3]);
        }
        else if (subcommand == "remove")
        {
            nodeRemove(argv[3]);
        }
        else
        {
            cout << "Not implemented\n";
        }
    }
    else
    {
        cout << "Not implemented\n";
    }

    return EXIT_SUCCESS;
}

