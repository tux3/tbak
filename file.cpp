#include "file.h"
#include "serialize.h"
#include "crc32.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace std;

File::File(const std::string& Path)
    : rawSize{0}, actualSize{0},
      crc32{0}, attrs{0,0,0,0}
{
    path = Path;
    readAttributes();
    computeCRC();
}

File::File(const std::vector<char>& data)
{
    deserialize(data);
}

void File::readAttributes()
{
    struct stat buf;
    if (stat(path.c_str(), &buf) < 0)
    {
        cerr << "WARNING: File::readAttributes: Failed to stat "<<path<<endl;
        return;
    }

    rawSize = actualSize = buf.st_size;
    actualSize += sizeof(File);
    attrs.mtime = (uint64_t)buf.st_mtim.tv_sec;
    attrs.userId = buf.st_uid;
    attrs.groupId = buf.st_gid;
    attrs.mode = buf.st_mode;
}

void File::computeCRC()
{
    ifstream f{path, ios_base::binary};
    if (f.is_open())
    {
        f.seekg(0, std::ios::end);
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        char* data = new char[size];
        f.read(data, size);
        f.close();

        crc32 = ::crc32(data, size);

        delete data;
    }
}

vector<char> File::serialize() const
{
    vector<char> data;

    serializeAppend(data, path);
    serializeAppend(data, rawSize);
    serializeAppend(data, actualSize);
    serializeAppend(data, crc32);
    serializeAppend(data, attrs.mtime);
    serializeAppend(data, attrs.userId);
    serializeAppend(data, attrs.groupId);
    serializeAppend(data, attrs.mode);

    return data;
}

void File::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    path = deserializeConsume<decltype(path)>(it);
    rawSize = deserializeConsume<decltype(rawSize)>(it);
    actualSize = deserializeConsume<decltype(actualSize)>(it);
    crc32 = deserializeConsume<decltype(crc32)>(it);
    attrs.mtime = deserializeConsume<decltype(attrs.mtime)>(it);
    attrs.userId = deserializeConsume<decltype(attrs.userId)>(it);
    attrs.groupId = deserializeConsume<decltype(attrs.groupId)>(it);
    attrs.mode = deserializeConsume<decltype(attrs.mode)>(it);
}
