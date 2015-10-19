#include "sourcefile.h"
#include "source.h"
#include "serialize.h"
#include "util/filelocker.h"
#include "util/pathtools.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <utime.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

using namespace std;

SourceFile::SourceFile(const Source* parent, const std::string& path)
    : pathHash{path}, parent{parent}, path{path}
{
    struct stat buf;
    if (stat((parent->getPath()+"/"+path).c_str(), &buf) < 0)
        throw runtime_error("SourceFile: Failed to stat "+path);

    attrs.mtime = (uint64_t)buf.st_mtim.tv_sec;
    attrs.userId = buf.st_uid;
    attrs.groupId = buf.st_gid;
    attrs.mode = buf.st_mode;
    rawSize = buf.st_size;
}

SourceFile::SourceFile(const Source* parent, const std::vector<char> &metadata,
                       uint64_t mtime, const std::vector<char> &data)
    : parent{parent}
{
    auto mit = metadata.begin();
    path = ::deserializeConsume<decltype(path)>(mit);
    attrs.userId = ::deserializeConsume<decltype(attrs.userId)>(mit);
    attrs.groupId = ::deserializeConsume<decltype(attrs.groupId)>(mit);
    attrs.mode = ::deserializeConsume<decltype(attrs.mode)>(mit);
    attrs.mtime = mtime;
    rawSize = data.size();

    string fullPath = parent->getPath()+'/'+path;
    createPathTo("/", fullPath);
    {
        FileLocker file{fullPath};
        file.overwrite(data);
    }

    applyAttrs();
}

uint64_t SourceFile::getRawSize() const
{
    return rawSize;
}

FileAttr SourceFile::getAttrs() const
{
    return attrs;
}

string SourceFile::getPath() const
{
    return path;
}

PathHash SourceFile::getPathHash() const
{
    return pathHash;
}

std::vector<char> SourceFile::readAll() const
{
    string fullpath = parent->getPath()+"/"+path;

    int fd = open(fullpath.c_str(), O_RDONLY);
    if (fd < 0)
        throw runtime_error("SourceFile::readAll: Unabled to open "+fullpath);

    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    vector<char> data(size);
    auto r = read(fd, data.data(), size);
    if (r<0)
        data.clear();

    close(fd);
    return data;
}

std::vector<char> SourceFile::serializeMetadata() const
{
    std::vector<char> data;

    ::serializeAppend(data, path);
    ::serializeAppend(data, attrs.userId);
    ::serializeAppend(data, attrs.groupId);
    ::serializeAppend(data, attrs.mode);

    return data;
}

bool SourceFile::operator <(const SourceFile &other) const
{
    return pathHash < other.pathHash;
}

void SourceFile::applyAttrs()
{
    string fullPath = parent->getPath()+'/'+path;
    chmod(fullPath.c_str(), attrs.mode);
    chown(fullPath.c_str(), attrs.userId, attrs.groupId);
    utimbuf times;
    times.actime = time(nullptr);
    times.modtime = attrs.mtime;
    utime(fullPath.c_str(), &times);
}

