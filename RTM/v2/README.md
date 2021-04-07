* RTM - Routing Table Manager with IP and ARP mapping

Add a new table of MAC addresses - a list of addresses and in **shared memory** keep
corresponding address of IP addresses.

Synchronization of MAC addresses need to be done using the UNIX socket sending to all clients
from RTM server process the update instructions with *OP code* **C** or **D**.

Each client and server has a copy or MAC address list and share the list of the counterpart IP addresses.

