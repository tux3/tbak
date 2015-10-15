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
    static std::string sha512str(std::string str);

    static void encrypt(std::vector<char> &data, const Server& s, const PublicKey &remoteKey);
    static void decrypt(std::vector<char>& data, const Server& s, const PublicKey &remoteKey);

    static void encryptPacket(NetPacket& packet, const Server& s, const PublicKey &remoteKey);
    static void decryptPacket(NetPacket& packet, const Server& s, const PublicKey &remoteKey);
};

#endif // CRYPTO_H
