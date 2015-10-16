#ifndef CRYPTO_H
#define CRYPTO_H

#include <array>
#include <vector>
#include <sodium.h>

using PublicKey = std::array<unsigned char, crypto_box_PUBLICKEYBYTES>;
using SecretKey = std::array<unsigned char, crypto_box_SECRETKEYBYTES>;

class NetPacket;
class Server;

class Crypto
{
public:
    Crypto();

    static void genkeys(PublicKey& pk, SecretKey& sk);
    static std::string keyToString(PublicKey key);
    static PublicKey stringToKey(std::string str);
    static std::vector<unsigned char> hash(std::string str);
    static std::string toBase64(std::vector<unsigned char> data);
    static std::string toBase64(unsigned char* data, size_t length);

    static void encrypt(std::vector<char> &data, const Server& s, const PublicKey &remoteKey);
    static void decrypt(std::vector<char>& data, const Server& s, const PublicKey &remoteKey);

    static void encryptPacket(NetPacket& packet, const Server& s, const PublicKey &remoteKey);
    static void decryptPacket(NetPacket& packet, const Server& s, const PublicKey &remoteKey);
};

#endif // CRYPTO_H
