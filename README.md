# tbak
A simple yet somewhat powerful backup tool I made in a weekend after my hard drive crashed.
The main thing I use it for is backing up source code. It supports compression, encryption, and per-backup synchronisation accross several nodes.

There are two important concepts, Folders and Nodes. 

A folder is a self-contained backup, it can be either a source or an archive.
A source folder contains just some metadata, whereas an archive folder contains metadata plus a compressed and encrypted copy of your file.

A node is a computer running tbak, when you add a remote node to your own local node, you basically grant that remote node full powers over your own node's backups, so only connect two nodes if you trust both of them.

Folders can be synchronised accross multiple nodes, and only the last compressed and encrypted version of every file will be backed-up on each node.
Each node has a full copy of each folder, but only the node that owns the source folder has the keys to decrypt the files.

### Example usage

Assuming that your local node is already linked to some remote nodes (the remote node added you, and you added it),
then this is how you would create a new backup and broadcast it to other nodes :

```
tbak folder add-source /some/folder/somewhere
tbak folder push /some/folder/somewhere
tbak folder sync /some/folder/somewhere
```

Then you would just run <code>tbak folder sync /some/folder/somewhere</code> any time you want to synchronise your folder with other nodes, this can for example be done as a cron job.

### Compiling

This code is not portable C++, it was written for Linux and uses several advanced non-portable features not available in standard C++ like directories and sockets...

Two libraries are used, zlib for compression and libsodium for encryption.
There is no Makefile, but a Qt .pro file is provived.<br/>
Compiling is done with <code>qmake && make</code>.<br/>
Or, simply passing all source files to g++ while linking <code>-lsodium -lz</code>
