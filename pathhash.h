#ifndef PATHHASH_H
#define PATHHASH_H

#include <string>
#include <vector>
#include <cstdint>

class PathHash
{
public:
    explicit PathHash();
    explicit PathHash(std::string str); ///< Construct from a string to be hashed
    explicit PathHash(uint8_t* data); ///< Read hash from serialized data
    explicit PathHash(const char* ambiguous) = delete; ///< Ambigous, string or serialized data?
    PathHash(const PathHash& other);
    std::string toBase64() const;

    bool operator==(const PathHash& other) const noexcept;
    bool operator<(const PathHash& other) const noexcept;

    std::vector<char> serialize() const;

public:
    static constexpr int hashlen = 18;
private:
    uint8_t hash[hashlen];
};

#endif // PATHHASH_H
