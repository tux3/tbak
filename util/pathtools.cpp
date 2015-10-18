#include "util/pathtools.h"
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

string normalizeFileName(const string& folder, const string& file)
{
    assert(!folder.empty() && !file.empty());
    string fileclean = file;
    if (file.size() > folder.size()
            && file.compare(folder) == 0)
    {
        fileclean = fileclean.substr(folder.size()+1);
    }
    return fileclean;
}

string normalizePath(const string &folder)
{
    string cleanstr;
    char* clean = realpath(folder.c_str(), nullptr);
    if (clean)
    {
        cleanstr = string(clean);
    }
    else
    {
        cleanstr = folder;
        while (*cleanstr.rbegin() == '/')
            cleanstr.resize(cleanstr.size()-1);
    }

    free(clean);
    return cleanstr;
}

void createDirectory(const char* path)
{
    mkdir(path, S_IRWXU | S_IRGRP | S_IWGRP);
}

void createDirectory(const string& path)
{
    createDirectory(path.c_str());
}

void createPathTo(const string &base, const string &relfile)
{
    size_t lastpos = 0;
    for (;;)
    {
        lastpos++;
        lastpos = relfile.find('/', lastpos);
        if (lastpos == string::npos)
            break;

        string next = relfile.substr(0, lastpos);
        createDirectory(base+"/"+next);
    }
}
