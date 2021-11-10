# Network stack

This is a simple network stack supporting IPv4, ARP, MAC, UDP, DHCP, TFTP. Most functions are zero-copy. To avoid bloating the project, target drivers (uart, timer, etc.) are intentionally left out. Only one example network driver is included. get_time() returns a 32-bit number which increments every millisecond. This is enough as long as we used relative times only.

## Overview

The network source file contains the main data structure that are being used. This is the network packet. This is pretty much passed to every single function. The concept is simple. The user allocates a network packet, reserve some space at the start of the buffer, and fills in the user data. The network packet then contain user data, and reserved bytes for the headers. Each layer will append its header in this region. At the mac layer, the packet is passed to the network driver. The gmac DMA will start the transfer from the start of the headers. I suggest going through in the following order:

- network.c 
  - contain network packet structure and an allocator
- mac.c
  - contain methods for appending the MAC header
  - this layer is the only layer that interacts with the physical driver. It calls this after appending the MAC header.
- arp.c
  - cooperates with the mac layer. If the IP is not known the mac.c will ask arp.c to resolve the mac address.
- ip.c
  - appends the IPv4 header and passes the packet to the mac layer.
- udp.c
  - appends the UDP header (port numbers) and passes the packet to the ip layer.
  - contain some methods for queuing packets.
- backoff.c
  - used for the two following protocols
  - used to track retransmission in case of lost packets
- tftp.c
  - uses udp.c
  - used to download files over network
  - support request, download, and error
- dhcp.c
  - uses udp.c
  - used to dynamically obtain/lease an IP address.
  - support discover, request, renewing, and rebinding
- icmp.c
  - uses ip.c
  - it only implements the ping protocol. Some inaccuracies might occur.
- utilities.c
  - random stuff like typedefs
  - formatted print (not relevant)
