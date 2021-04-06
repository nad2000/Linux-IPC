* RTM - Routing Table Manager with IP and ARP mapping

Added feature of signalling

Now, when a client gets connected to the server process, and Synchronization is finished, this new client sends
its own process ID (*getpid()*) to the routing manager server process using UNIX domain sockets.

Server stores the PIDs of the all clients in a list of something;

Now, add a choice in Menu on Sever side choosing which Server flushes off its entire routing table and ARP table
in shared memory;

After flushing, the server sends SIGUSR1 signal to all the connected clients using their PIDs with *kill()*. Having received this signals, clients are supposed to flush off their won routing tables and ARP table copies.

Also, whenever the client terminates the UNIX domain connections with the server, the client should clean all its tables it is maintaining. Before closing the connection client should send one last message to the server.
Server having received that last message should remove PID of this client from its list.
