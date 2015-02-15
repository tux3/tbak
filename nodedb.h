#ifndef NODEDB_H
#define NODEDB_H

#include <string>
#include <vector>
#include <sodium.h>
#include "node.h"

/// Maintains a database of Nodes
class NodeDB
{
public:
    NodeDB(const std::string& path);  ///< Reads serialized data from file
    NodeDB(const std::vector<char> &data);

    void load(const std::string& path);
    void deserialize(const std::vector<char>& data);
    void save(const std::string& path) const;
    std::vector<char> serialize() const;

    const std::vector<Node>& getNodes() const;
    void addNode(const std::string& uri);
    bool removeNode(const std::string& uri);

private:
    std::vector<Node> nodes;
};

#endif // NODEDB_H
