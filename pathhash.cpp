#include "pathhash.h"
#include "crypto.h"
#include "cassert"

using namespace std;

PathHash::PathHash()
    : hash{0}
{
}

PathHash::PathHash(const string &str)
{
    rehash(str);
}

PathHash::PathHash(uint8_t *data)
{
    copy(data, data+hashlen, hash);
}

PathHash::PathHash(const PathHash &other)
{
    copy(other.hash, other.hash+hashlen, hash);
}

std::string PathHash::toBase64() const
{
    return Crypto::toBase64(hash, hashlen);
}

void PathHash::rehash(const string &str)
{
    Crypto::hashInto(str, hash);
}

bool PathHash::operator==(const PathHash &other) const noexcept
{
    return equal(hash, hash+hashlen, other.hash);
}

bool PathHash::operator<(const PathHash &other) const noexcept
{
    return lexicographical_compare(hash, hash+hashlen, other.hash, other.hash+hashlen);
}

std::vector<char> PathHash::serialize() const
{
    return vector<char>(hash, hash+hashlen);
}


void PathHash::serializeInto(std::vector<char> &dest) const
{
    size_t size = dest.size();
    dest.resize(size+hashlen);
    copy(&hash[0], &hash[hashlen], &dest[size]);
}
