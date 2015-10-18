#include "archivefile.h"
#include "archive.h"
#include "util/filelocker.h"
#include "util/pathtools.h"
#include "serialize.h"

using namespace std;

ArchiveFile::ArchiveFile(const Archive *parent, PathHash pathHash, uint64_t mtime, const std::vector<char> &data)
    : pathHash{pathHash}, mtime{mtime}, actualSize{data.size()}, parent{parent}
{
    overwrite(mtime, data);
}

ArchiveFile::ArchiveFile(const Archive *parent, vector<char>::const_iterator &serializedData)
    : parent{parent}
{

    pathHash = ::deserializeConsume<decltype(pathHash)>(serializedData);
    mtime = ::deserializeConsume<decltype(mtime)>(serializedData);
    actualSize = ::deserializeConsume<decltype(actualSize)>(serializedData);
}

PathHash ArchiveFile::getPathHash() const
{
    return pathHash;
}

uint64_t ArchiveFile::getMtime() const
{
    return mtime;
}

uint64_t ArchiveFile::getActualSize() const
{
    return actualSize;
}

std::vector<char> ArchiveFile::read(uint64_t startPos, uint64_t size) const
{
    string pathHashStr = pathHash.toBase64();
    string fullPath = parent->getFolderDataPath()+'/'+pathHashStr.substr(0,2)+'/'+pathHashStr.substr(2);
    FileLocker file{fullPath};
    return file.read(startPos, size);
}

std::vector<char> ArchiveFile::readMetadata() const
{
    std::vector<char> meta;
    std::vector<char> msizeData = read(0, sizeof(size_t));
    if (msizeData.size() < sizeof(size_t))
        return meta;
    auto mit = msizeData.cbegin();
    size_t msize = ::dataToVUint(mit);
    size_t msizeSize = ::getVUint32Size(msizeData);

    meta = read(msizeSize, msize);
    return meta;
}

std::vector<char> ArchiveFile::readAll() const
{
    string pathHashStr = pathHash.toBase64();
    string fullPath = parent->getFolderDataPath()+'/'+pathHashStr.substr(0,2)+'/'+pathHashStr.substr(2);
    FileLocker file{fullPath};
    return file.readAll();
}

void ArchiveFile::overwrite(uint64_t _mtime, const std::vector<char> &data)
{
    mtime = _mtime;
    actualSize = data.size();

    string pathHashStr = pathHash.toBase64();
    string fullPath = parent->getFolderDataPath()+'/'+pathHashStr.substr(0,2)+'/'+pathHashStr.substr(2);
    createPathTo(parent->getFolderDataPath(), pathHashStr.substr(0,2)+'/'+pathHashStr.substr(2));
    FileLocker file{fullPath};
    file.overwrite(data);
}

std::vector<char> ArchiveFile::serialize() const
{
    std::vector<char> data;
    ::serializeAppend(data, pathHash);
    ::serializeAppend(data, mtime);
    ::serializeAppend(data, actualSize);
    return data;
}

string ArchiveFile::deserializePath(std::vector<char>::const_iterator& meta)
{
    return ::dataToString(meta);
}
