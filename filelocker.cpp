#include "filelocker.h"
#include <cstdlib>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <stdexcept>

using namespace std;

FileLocker::FileLocker(const string &path)
{
    fd = open(path.c_str(), O_RDWR | O_CREAT);
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

std::vector<char> FileLocker::readAll() const
{
    lock_guard<std::mutex> lock(mutex);
    size_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    vector<char> data(size);
    read(fd, data.data(), size);
    return data;
}

void FileLocker::truncate() const
{
    lock_guard<std::mutex> lock(mutex);
    ftruncate(fd, 0);
}

void FileLocker::write(const std::vector<char>& data) const
{
    lock_guard<std::mutex> lock(mutex);
    ::write(fd, data.data(), data.size());
}

void FileLocker::overwrite(const std::vector<char>& data) const
{
    lock_guard<std::mutex> lock(mutex);
    truncate();
    write(data);
}
