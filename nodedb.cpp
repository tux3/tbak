#include "nodedb.h"
#include "serialize.h"
#include "net.h"
#include <fstream>
#include <algorithm>
#include <iostream>

using namespace std;

NodeDB::NodeDB(const string &uri)
{
    load(uri);
}

NodeDB::NodeDB(const vector<char> &data)
{
    if (!data.empty())
        deserialize(data);
}

void NodeDB::save(const string &path) const
{
    vector<char> data = serialize();
    ofstream f(path, ios_base::binary | ios_base::trunc);
    if (f.is_open())
    {
        copy(begin(data), end(data), ostreambuf_iterator<char>(f));
        f.close();
    }
}

vector<char> NodeDB::serialize() const
{
    vector<char> data;

    vector<vector<char>> nodesData;
    for (const Node& n : nodes)
        nodesData.push_back(n.serialize());

    serializeAppend(data, nodesData);

    return data;
}

void NodeDB::load(const std::string& path)
{
    ifstream f(path, ios_base::binary);
    vector<char> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (f.is_open())
        f.close();

    if (!data.empty())
        deserialize(data);
}

void NodeDB::deserialize(const std::vector<char> &data)
{
    auto it = begin(data);

    vector<vector<char>> nodesData = deserializeConsume<decltype(nodesData)>(it);
    for (const vector<char>& vec : nodesData)
        nodes.push_back(Node(vec));
}

const std::vector<Node>& NodeDB::getNodes() const
{
    return nodes;
}

void NodeDB::addNode(const std::string& uri)
{
    auto pred = [&uri](const Node& n){return n.getUri() == uri;};
    if (find_if(begin(nodes), end(nodes), pred) != end(nodes))
        return;

    auto pk = Net::getNodePk(uri);

    cout << "Received pk is "<<Crypto::keyToString(pk)<<endl;

    nodes.push_back(Node(uri, pk));
}

bool NodeDB::removeNode(const std::string& uri)
{
    for (unsigned i=0; i<nodes.size(); ++i)
    {
        if (nodes[i].getUri() == uri)
        {
            nodes.erase(begin(nodes)+i);
            return true;
        }
    }
    return false;
}
