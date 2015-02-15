#ifndef FILELOCKER_H
#define FILELOCKER_H

#include <string>
#include <vector>
#include <mutex>

///< Acquires a shared lock on a file and release it on destruction
///< Throws if the lock can't be acquired
class FileLocker
{
public:
    FileLocker(const std::string& path);
    ~FileLocker();

    std::vector<char> readAll() const noexcept;

    bool truncate() const noexcept;
    bool write(const std::vector<char>& data) const noexcept;
    bool overwrite(const std::vector<char>& data) const noexcept; ///< Truncate then write

private:
    int fd;
    mutable std::mutex mutex;
};

#endif // FILELOCKER_H
