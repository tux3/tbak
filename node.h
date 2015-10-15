#ifndef NODE_H
#define NODE_H

#include <vector>
#include <string>
#include <array>

#include "crypto.h"

class Node
{
public:
    Node(const std::string& uri, PublicKey pk);  ///< Add node with this URI
    Node(const std::vector<char>& data); ///< Construct from serialized data

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);

    const std::string& getUri() const;
    std::string getPkString() const;
    const PublicKey& getPk() const;

private:
    std::string uri;
    PublicKey pk;
};

#endif // NODE_H
