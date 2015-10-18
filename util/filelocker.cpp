#include "util/filelocker.h"
#include <cstdlib>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <stdexcept>

using namespace std;

FileLocker::FileLocker(const string &path)
    : path{path}
{
    fd = open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
        throw runtime_error("FileLocker::FileLocker: Unabled to open "+path);

    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
        throw runtime_error("FileLocker::FileLocker: Unable to lock  "+path);

}

FileLocker::~FileLocker()
{
    flock(fd, LOCK_UN);
    close(fd);
}

std::vector<char> FileLocker::read(uint64_t startPos, uint64_t size) const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    size_t fsize = lseek(fd, 0, SEEK_END);
    if (fsize < startPos)
        return {};
    if (fsize < startPos + size)
        size = startPos - fsize;
    lseek(fd, startPos, SEEK_SET);

    vector<char> data(size);
    auto r = ::read(fd, data.data(), size);
    if (r<0)
        data.clear();
    return data;
}

std::vector<char> FileLocker::readAll() const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    vector<char> data(size);
    auto r = ::read(fd, data.data(), size);
    if (r<0)
        data.clear();
    return data;
}

bool FileLocker::remove() const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    return std::remove(path.c_str()) == 0;
}

bool FileLocker::truncate() const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    lseek(fd, 0, SEEK_SET);
    return ftruncate(fd, 0) == 0;
}

bool FileLocker::write(const char* data, size_t size) const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    auto result = ::write(fd, data, size);
    return (result>0 && (size_t)result == size);
}

bool FileLocker::write(const std::vector<char>& data) const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    auto result = ::write(fd, data.data(), data.size());
    return (result>0 && (size_t)result == data.size());
}

bool FileLocker::overwrite(const char* data, size_t size) const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    if (!truncate())
        return false;
    return write(data, size);
}

bool FileLocker::overwrite(const std::vector<char>& data) const noexcept
{
    lock_guard<std::mutex> lock(mutex);
    if (!truncate())
        return false;
    return write(data);
}
