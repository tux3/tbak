#include "sighandlers.h"
#include "server.h"
#include <iostream>

static void sigtermHandler(int)
{
    Server::abortall = true;
    std::cout << "Caught signal, exiting gracefully..."<<std::endl;
}

void setupSigHandlers()
{
    struct sigaction sigactionHandler;
    sigactionHandler.sa_handler = sigtermHandler;
    sigemptyset(&sigactionHandler.sa_mask);
    sigactionHandler.sa_flags = 0;
    sigaction(SIGINT, &sigactionHandler, nullptr);
    sigaction(SIGTERM, &sigactionHandler, nullptr);
}
