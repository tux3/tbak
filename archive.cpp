#include "archive.h"
#include "serialize.h"
#include "settings.h"
#include "filelocker.h"
#include "crypto.h"
#include "compression.h"
#include "pathtools.h"
#include <dirent.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <limits.h>

using namespace std;

Archive::Archive(const Archive& other)
{
    *this = other;
}

Archive::Archive(PathHash pathHash)
    : pathHash{pathHash}
{
    actualSize = 0;
    createDirectory(getFolderDataPath());
}

Archive::Archive(const std::vector<char>& data)
{
    deserialize(data);
    createDirectory(getFolderDataPath());
}

Archive::~Archive()
{
}

Archive& Archive::operator=(const Archive& other)
{
    lock_guard<std::recursive_mutex> lock(mutex);
    lock_guard<std::recursive_mutex> lockother(other.mutex);
    pathHash = other.pathHash;
    actualSize = other.actualSize;
    files = other.files;
    return *this;
}

std::vector<std::string> Archive::listfiles(const char *name, int level) const
{
    assert(name[strlen(name)-1] != '/');
    std::vector<std::string> files;
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return files;
    if (!(entry = readdir(dir)))
        return files;

    do
    {
        if (entry->d_type == DT_DIR)
        {
            char path[PATH_MAX];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            vector<string> newfiles = listfiles(path, level + 1);
            files.insert(end(files), begin(newfiles), end(newfiles));
        }
        else if (entry->d_type == DT_REG)
        {
            char path[PATH_MAX];
            int len = snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            std::string s(path, len);
            files.push_back(s);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    return files;
}

void Archive::deleteFolderRecursively(const char* name) const
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do
    {
        if (entry->d_type == DT_DIR)
        {
            char path[PATH_MAX];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            deleteFolderRecursively(path);
        }
        else
        {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            unlink(path);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    rmdir(name);
}

vector<char> Archive::serialize() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    vector<char> data;

    serializeAppend(data, pathHash);
    serializeAppend(data, actualSize);
    for (const ArchiveFile& file : files)
        vectorAppend(data, file.serialize());

    return data;
}

void Archive::deserialize(const std::vector<char>& data)
{
    lock_guard<std::recursive_mutex> lock(mutex);
    auto it = data.begin();
    if (distance(it, data.end()) < (long)(PathHash::hashlen + sizeof(actualSize)))
        throw runtime_error("Archive::deserialize: Invalid serialized metadata\n");
    pathHash = deserializeConsume<decltype(pathHash)>(it);
    actualSize = deserializeConsume<decltype(actualSize)>(it);
    size_t elemSize = ArchiveFile::serializedSize();
    if (distance(it, data.end()) % elemSize != 0)
        throw runtime_error("Archive::deserialize: Invalid serialized data\n");
    for (int i=distance(it, data.end()) / elemSize; i; --i)
        files.emplace_back(this, it);
}

PathHash Archive::getPathHash() const
{
    return pathHash;
}

uint64_t Archive::getActualSize() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    return actualSize;
}

size_t Archive::getFileCount() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    return files.size();
}

ArchiveFile *Archive::getFile(PathHash pathHash)
{
    auto it = find_if(begin(files), end(files), [&pathHash](const ArchiveFile& f){return f.getPathHash()==pathHash;});
    if (it == end(files))
        return nullptr;
    return &*it;
}

const std::vector<ArchiveFile> &Archive::getFiles() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    return files;
}

std::string Archive::getFilesDbPath() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    return dataPath()+"archive/"+pathHash.toBase64()+"/files.dat";
}

std::string Archive::getFolderDataPath() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    return dataPath()+"archive/"+pathHash.toBase64();
}

void Archive::removeData() const
{
    lock_guard<std::recursive_mutex> lock(mutex);
    deleteFolderRecursively((dataPath()+"archive/"+pathHash.toBase64()).c_str());
}

void Archive::writeArchiveFile(const PathHash& filePath, uint64_t mtime, const std::vector<char>& data)
{
    lock_guard<std::recursive_mutex> lock(mutex);

    auto it = find_if(begin(files), end(files), [&filePath](const ArchiveFile& f){return f.getPathHash()==filePath;});

    if (it == end(files))
    {
        actualSize += data.size();
        files.emplace_back(this, filePath, mtime, data);
    }
    else
    {
        actualSize -= it->getActualSize();
        actualSize += data.size();
        it->overwrite(mtime, data);
    }
}

bool Archive::removeArchiveFile(const PathHash& pathHash)
{
    string path = pathHash.toBase64();
    string hashdirPath = getFolderDataPath()+"/"+path.substr(0,2);
    string hashfilePath = hashdirPath+"/"+path.substr(2);

    auto it = find_if(begin(files), end(files), [=](const ArchiveFile& f)
    {
        return f.getPathHash()==pathHash;
    });
    if (it == end(files))
        return false;

    actualSize -= it->getActualSize();
    files.erase(it);

    try {
        FileLocker filel(hashfilePath);
        return filel.remove();
    } catch (...)
    {
        cout << "Folder::removeArchiveFile: File "<<hashfilePath<<" not found"<<endl;
        return false;
    }
}

