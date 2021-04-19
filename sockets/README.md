There are 2 **kind** of messages (or requests) sent by the client to the server:

 - Connection initiation request messages:
 	- The client request the server to establish a dedicated connection (at the client side -  system call *conncet()*, at the serer side - *accept()*);
	- Only after the connection has been established, then only client can send Service request message to the server (at the client side - *sednmsg()*, *sendto()*; at the server - *recvmsg()*, *recvfrom()*) .

 - Service Request Messages:
 	- Client can send theses type of messages once the connection is fully established;
	- Through these messages, the Client request server to provide a service.


The Life Cycle of the Client Server Communication:

 - When the server boots up, it creates a **master socket** using *socket()*;
 - When client issues an new connection request sent to the server master socket;
 - The server creates a new **client handle** using *accept()* (each client gets its own handle);
 - The service request would be sent to the **client handle** (not the **master socket**);

