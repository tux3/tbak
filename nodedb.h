#ifndef NODEDB_H
#define NODEDB_H

#include <string>
#include <vector>
#include "node.h"
#include "util/filelocker.h"

/// Maintains a database of Nodes
class NodeDB
{
public:
    NodeDB(const std::string& path);  ///< Reads serialized data from file
    ~NodeDB(); ///< Saves automatically

    void load();
    void deserialize(const std::vector<char>& data);
    void save() const;
    std::vector<char> serialize() const;

    const std::vector<Node>& getNodes() const;
    void addNode(const std::string& uri);
    void addNode(const std::string& uri, const std::string& pk);
    bool removeNode(const std::string& uri);

private:
    std::vector<Node> nodes;
    FileLocker file;
};

#endif // NODEDB_H
