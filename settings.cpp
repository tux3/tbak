#include "settings.h"

const std::string getHomePath()
{
    static std::string homepath = getenv("HOME");
    return homepath;
}

const std::string& dataPath()
{
    static std::string path = getHomePath() + "/.tbak/";
    return path;
}

const std::string& folderDBPath()
{
    static std::string path = getHomePath() + "/.tbak/folders.dat";
    return path;
}

const std::string& nodeDBPath()
{
    static std::string path = getHomePath() + "/.tbak/nodes.dat";
    return path;
}

const std::string& serverConfigPath()
{
    static std::string path = getHomePath() + "/.tbak/server.dat";
    return path;
}

const int PORT_NUMBER = 6700;
const char* PORT_NUMBER_STR = "6700";
