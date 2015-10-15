#include "node.h"
#include "serialize.h"

using namespace std;

Node::Node(const std::string& Uri, PublicKey Pk)
    : uri{Uri}
{
    pk = Pk;
}

Node::Node(const std::vector<char>& data)
{
    deserialize(data);
}

std::vector<char> Node::serialize() const
{
    vector<char> data;

    serializeAppend(data, uri);
    serializeAppend(data, pk);

    return data;
}

void Node::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    uri = deserializeConsume<decltype(uri)>(it);
    pk = deserializeConsume<decltype(pk)>(it);
}

const std::string& Node::getUri() const
{
    return uri;
}

std::string Node::getPkString() const
{
    return Crypto::keyToString(pk);
}

const PublicKey& Node::getPk() const
{
    return pk;
}
