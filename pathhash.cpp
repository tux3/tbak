#include "pathhash.h"
#include "crypto.h"
#include "cassert"

using namespace std;

PathHash::PathHash()
    : hash{0}
{
}

PathHash::PathHash(std::string str)
{
    vector<unsigned char> strhash = Crypto::hash(str);
    uint8_t* data = reinterpret_cast<uint8_t*>(strhash.data());
    assert(strhash.size() == hashlen);
    copy(data, data+hashlen, hash);
}

PathHash::PathHash(char *data)
{
    copy(data, data+hashlen, hash);
}

PathHash::PathHash(const PathHash &other)
{
    copy(other.hash, other.hash+hashlen, hash);
}

std::string PathHash::toBase64()
{
    return Crypto::toBase64(hash, hashlen);
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
