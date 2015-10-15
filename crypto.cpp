#include "crypto.h"
#include "server.h"
#include "netpacket.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>

Crypto::Crypto()
{

}

int rawencrypt(char encrypted[], const uint8_t pk[], const uint8_t sk[], const char nonce[], const char plain[], int length)
{
    if (crypto_box_easy((uint8_t*)encrypted, (uint8_t*)plain, length, (uint8_t*)nonce, pk, sk))
        throw std::runtime_error("Crypto encrypt: Encryption failed");

    return length;
}

int rawdecrypt(char plain[], const uint8_t pk[], const uint8_t sk[], const char nonce[], const char encrypted[], int length)
{
    if (crypto_box_open_easy((uint8_t*)plain, (uint8_t*)encrypted, length, (uint8_t*)nonce, pk, sk))
        throw std::runtime_error("Crypto decrypt: Invalid or forged cyphertext");

    return length;
}

void Crypto::genkeys(PublicKey &pk, SecretKey &sk)
{
    crypto_box_keypair(&pk[0],&sk[0]);
}

std::string Crypto::keyToString(PublicKey key)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = sizeof(key);

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = key[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

void Crypto::encryptPacket(NetPacket& packet, const Server &s, const PublicKey& remoteKey)
{
    if (!packet.data.size())
        return;
    size_t encryptedsize = packet.data.size()+crypto_box_NONCEBYTES+crypto_box_MACBYTES;
    std::vector<char> encrypted(encryptedsize);
    randombytes((uint8_t*)&encrypted[0], crypto_box_NONCEBYTES);
    rawencrypt(&encrypted[crypto_box_NONCEBYTES], &remoteKey[0], &s.getSecretKey()[0], &encrypted[0],
            &packet.data[0], packet.data.size());
    packet.data = encrypted;
}

void Crypto::decryptPacket(NetPacket& packet, const Server &s, const PublicKey& remoteKey)
{
    if (!packet.data.size())
            return;
    else if (packet.data.size() < crypto_box_NONCEBYTES+crypto_box_MACBYTES)
        throw std::runtime_error("Crypto::decryptPacket: Packet is too short, can't decrypt");
    size_t plainsize = packet.data.size()-crypto_box_NONCEBYTES-crypto_box_MACBYTES;
    std::vector<char> plaintext(plainsize);
    char* nonce = &packet.data[0], *encrypted=&packet.data[0]+crypto_box_NONCEBYTES;
    rawdecrypt(&plaintext[0], &remoteKey[0], &s.getSecretKey()[0], nonce, encrypted, packet.data.size()-crypto_box_NONCEBYTES);
    packet.data = plaintext;
}

void Crypto::encrypt(std::vector<char>& data, const Server& s, const PublicKey &remoteKey)
{
    if (!data.size())
        return;
    size_t encryptedsize = data.size()+crypto_box_NONCEBYTES+crypto_box_MACBYTES;
    std::vector<char> encrypted(encryptedsize);
    randombytes((uint8_t*)&encrypted[0], crypto_box_NONCEBYTES);
    rawencrypt(&encrypted[crypto_box_NONCEBYTES], &remoteKey[0], &s.getSecretKey()[0], &encrypted[0],
            &data[0], data.size());
    data = encrypted;
}

void Crypto::decrypt(std::vector<char> &data, const Server& s, const PublicKey &remoteKey)
{
    if (!data.size())
            return;
    else if (data.size() < crypto_box_NONCEBYTES+crypto_box_MACBYTES)
        throw std::runtime_error("Crypto::decryptPacket: Packet is too short, can't decrypt");
    size_t plainsize = data.size()-crypto_box_NONCEBYTES-crypto_box_MACBYTES;
    std::vector<char> plaintext(plainsize);
    char* nonce = &data[0], *encrypted=&data[0]+crypto_box_NONCEBYTES;
    rawdecrypt(&plaintext[0], &remoteKey[0], &s.getSecretKey()[0], nonce, encrypted, data.size()-crypto_box_NONCEBYTES);
    data = plaintext;
}

std::vector<unsigned char> Crypto::hash(std::__cxx11::string str)
{
    static constexpr int hashlen = 21;
    std::vector<unsigned char> h(hashlen);

    crypto_generichash(h.data(), hashlen, (const unsigned char*)str.data(), str.size(), nullptr, 0);
    return h;
}

std::string Crypto::toBase64(std::vector<unsigned char> data)
{
    static constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    static constexpr char padding = '=';
    std::string encodedString;
    encodedString.reserve(((data.size()/3) + (data.size() % 3 > 0)) * 4);
    uint32_t temp;
    auto cursor = data.begin();
    for(size_t idx = 0; idx < data.size()/3; idx++)
    {
        temp  = (*cursor++) << 16; //Convert to big endian
        temp += (*cursor++) << 8;
        temp += (*cursor++);
        encodedString.append(1,charset[(temp & 0x00FC0000) >> 18]);
        encodedString.append(1,charset[(temp & 0x0003F000) >> 12]);
        encodedString.append(1,charset[(temp & 0x00000FC0) >> 6 ]);
        encodedString.append(1,charset[(temp & 0x0000003F)      ]);
    }
    switch(data.size() % 3)
    {
    case 1:
        temp  = (*cursor++) << 16; //Convert to big endian
        encodedString.append(1,charset[(temp & 0x00FC0000) >> 18]);
        encodedString.append(1,charset[(temp & 0x0003F000) >> 12]);
        encodedString.append(2,padding);
        break;
    case 2:
        temp  = (*cursor++) << 16; //Convert to big endian
        temp += (*cursor++) << 8;
        encodedString.append(1,charset[(temp & 0x00FC0000) >> 18]);
        encodedString.append(1,charset[(temp & 0x0003F000) >> 12]);
        encodedString.append(1,charset[(temp & 0x00000FC0) >> 6 ]);
        encodedString.append(1,padding);
        break;
    }
    return encodedString;
}
