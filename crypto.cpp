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

void randombytes(char buffer[], unsigned long long size)
{
    int fd = open( "/dev/urandom", O_RDONLY );
    if(fd < 0)
        throw std::runtime_error("Failed to open /dev/urandom!");
    int rc;
    if((rc = read(fd, buffer, size)) >= 0)
        close(fd);
}

int rawencrypt(char encrypted[], const uint8_t pk[], const uint8_t sk[], const char nonce[], const char plain[], int length)
{
    uint8_t* temp_encrypted = new uint8_t[length+crypto_box_ZEROBYTES];
    uint8_t* temp_plain = new uint8_t[length+crypto_box_ZEROBYTES];

    memset(temp_encrypted, '\0', crypto_box_BOXZEROBYTES);
    memset(temp_plain, '\0', crypto_box_ZEROBYTES);
    memcpy(temp_plain + crypto_box_ZEROBYTES, plain, length);

    if (crypto_box(temp_encrypted, temp_plain, crypto_box_ZEROBYTES + length, (uint8_t*)nonce, pk, sk))
        throw std::runtime_error("Crypto encrypt: Encryption failed");

    memcpy(encrypted, temp_encrypted + crypto_box_BOXZEROBYTES, crypto_box_BOXZEROBYTES+length);
    delete[] temp_plain;
    delete[] temp_encrypted;

    return length;
}

int rawdecrypt(char plain[], const uint8_t pk[], const uint8_t sk[], const char nonce[], const char encrypted[], int length)
{
    uint8_t* temp_encrypted = new uint8_t[length+crypto_box_ZEROBYTES];
    uint8_t* temp_plain = new uint8_t[length+crypto_box_ZEROBYTES];

    memset(temp_encrypted, '\0', crypto_box_BOXZEROBYTES);
    memcpy(temp_encrypted + crypto_box_BOXZEROBYTES, encrypted, length);

    if (crypto_box_open(temp_plain, temp_encrypted, crypto_box_BOXZEROBYTES + length, (uint8_t*)nonce, pk, sk))
        throw std::runtime_error("Crypto decrypt: Invalid or forged cyphertext");

    memcpy(plain, temp_plain + crypto_box_ZEROBYTES, length);
    delete[] temp_plain;
    delete[] temp_encrypted;

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
    size_t encryptedsize = packet.data.size()+crypto_box_NONCEBYTES+crypto_box_BOXZEROBYTES;
    std::vector<char> encrypted(encryptedsize);
    randombytes(&encrypted[0], crypto_box_NONCEBYTES);
    rawencrypt(&encrypted[crypto_box_NONCEBYTES], &remoteKey[0], &s.getSecretKey()[0], &encrypted[0],
            &packet.data[0], packet.data.size());
    packet.data = encrypted;
}

void Crypto::decryptPacket(NetPacket& packet, const Server &s, const PublicKey& remoteKey)
{
    if (!packet.data.size())
            return;
    else if (packet.data.size() < crypto_box_NONCEBYTES)
        throw std::runtime_error("Crypto::decryptPacket: Packet doesn't have a nonce, can't decrypt");
    size_t plainsize = packet.data.size()-crypto_box_NONCEBYTES;
    std::vector<char> plaintext(plainsize);
    char* nonce = &packet.data[0], *encrypted=&packet.data[0]+crypto_box_NONCEBYTES;
    rawdecrypt(&plaintext[0], &remoteKey[0], &s.getSecretKey()[0], nonce, encrypted, plainsize);
    plaintext.resize(plainsize-crypto_box_BOXZEROBYTES);
    packet.data = plaintext;
}

void Crypto::encrypt(std::vector<char>& data, const Server& s, const PublicKey &remoteKey)
{
    if (!data.size())
        return;
    size_t encryptedsize = data.size()+crypto_box_NONCEBYTES+crypto_box_BOXZEROBYTES;
    std::vector<char> encrypted(encryptedsize);
    randombytes(&encrypted[0], crypto_box_NONCEBYTES);
    rawencrypt(&encrypted[crypto_box_NONCEBYTES], &remoteKey[0], &s.getSecretKey()[0], &encrypted[0],
            &data[0], data.size());
    data = encrypted;
}

void Crypto::decrypt(std::vector<char> &data, const Server& s, const PublicKey &remoteKey)
{
    if (!data.size())
            return;
    else if (data.size() < crypto_box_NONCEBYTES)
        throw std::runtime_error("Crypto::decryptPacket: Packet doesn't have a nonce, can't decrypt");
    size_t plainsize = data.size()-crypto_box_NONCEBYTES;
    std::vector<char> plaintext(plainsize);
    char* nonce = &data[0], *encrypted=&data[0]+crypto_box_NONCEBYTES;
    rawdecrypt(&plaintext[0], &remoteKey[0], &s.getSecretKey()[0], nonce, encrypted, plainsize);
    plaintext.resize(plainsize-crypto_box_BOXZEROBYTES);
    data = plaintext;
}

std::string Crypto::sha512str(std::string str)
{
    unsigned char h[crypto_hash_BYTES];
    crypto_hash(h, (const unsigned char *) str.c_str(), str.size());
    char h_hex[crypto_hash_BYTES + 1];
    sodium_bin2hex(h_hex, sizeof h_hex, h, sizeof h);

    return std::string(h_hex);
}
