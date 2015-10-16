#ifndef PATHHASH_H
#define PATHHASH_H

#include <string>
#include <vector>
#include <cstdint>

class PathHash
{
public:
    PathHash();
    PathHash(std::string str);
    PathHash(const PathHash& other);
    std::string toBase64();

    bool operator==(const PathHash& other) const noexcept;
    bool operator<(const PathHash& other) const noexcept;

    std::vector<char> serialize() const;

private:
    static constexpr int hashlen = 18;
    uint8_t hash[hashlen];
};

#endif // PATHHASH_H
