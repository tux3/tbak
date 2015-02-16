#include "file.h"
#include "folder.h"
#include "serialize.h"
#include "crc32.h"
#include "sha512.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdexcept>
#include <cassert>

using namespace std;

File::File(const File& other)
{
    *this = other;
}

File::File(const Folder *parent, const std::string& Path)
    : parent{parent},
      rawSize{0}, actualSize{0},
       attrs{0,0,0,0}
{
    path = Path;
    //computeCRC();
    readAttributes();
}

File::File(const Folder *parent, const std::vector<char>& data)
    : parent{parent}
{
    deserialize(data);
}

File::File(const Folder* parent, std::vector<char>::const_iterator& data)
    : parent{parent}
{
    deserialize(data);
}

File& File::operator=(const File& other)
{
    lock_guard<std::mutex> lock(mutex);
    lock_guard<std::mutex> lockother(other.mutex);
    parent = other.parent;
    path = other.path;
    rawSize = other.rawSize;
    actualSize = other.actualSize;
    attrs = other.attrs;
    return *this;
}

void File::readAttributes()
{
    assert(parent->getType() == FolderType::Source);

    struct stat buf;
    if (stat((parent->getPath()+"/"+path).c_str(), &buf) < 0)
    {
        cerr << "WARNING: File::readAttributes: Failed to stat "<<path<<endl;
        return;
    }

    attrs.mtime = (uint64_t)buf.st_mtim.tv_sec;
    attrs.userId = buf.st_uid;
    attrs.groupId = buf.st_gid;
    attrs.mode = buf.st_mode;
    rawSize = actualSize = buf.st_size;
    actualSize += metadataSize();
}

vector<char> File::serialize() const
{
    lock_guard<std::mutex> lock(mutex);

    vector<char> data;

    serializeAppend(data, path);
    serializeAppend(data, rawSize);
    serializeAppend(data, actualSize);
    serializeAppend(data, attrs.mtime);
    serializeAppend(data, attrs.userId);
    serializeAppend(data, attrs.groupId);
    serializeAppend(data, attrs.mode);

    return data;
}

void File::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    deserialize(it);
}

void File::deserialize(std::vector<char>::const_iterator& it)
{
    lock_guard<std::mutex> lock(mutex);

    path = deserializeConsume<decltype(path)>(it);
    rawSize = deserializeConsume<decltype(rawSize)>(it);
    actualSize = deserializeConsume<decltype(actualSize)>(it);
    attrs.mtime = deserializeConsume<decltype(attrs.mtime)>(it);
    attrs.userId = deserializeConsume<decltype(attrs.userId)>(it);
    attrs.groupId = deserializeConsume<decltype(attrs.groupId)>(it);
    attrs.mode = deserializeConsume<decltype(attrs.mode)>(it);
}

std::vector<char> File::readAll() const
{
    lock_guard<std::mutex> lock(mutex);

    string fullpath;
    if (parent->getType() == FolderType::Source)
    {
        fullpath = parent->getPath()+"/"+path;
    }
    else if (parent->getType() == FolderType::Archive)
    {
        string newhash;
        sha512str(path.c_str(), path.size(), newhash);
        fullpath = parent->getFolderDataPath()+"/"+newhash.substr(0,2)+"/"+newhash.substr(2);
    }
    else
        throw std::runtime_error("File::readAll: Unknown parent folder type");

    int fd = open(fullpath.c_str(), O_RDONLY);
    if (fd < 0)
        throw runtime_error("File::readAll: Unabled to open "+fullpath);

    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    vector<char> data(size);
    auto r = read(fd, data.data(), size);
    if (r<0)
        data.clear();

    close(fd);
    return data;
}

size_t File::metadataSize()
{
    return serialize().size();
}
