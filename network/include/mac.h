// Author: strawberryhacker

#ifndef MAC_H
#define MAC_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

enum {
    ETHER_TYPE_IPV4 = 0x0800,
    ETHER_TYPE_ARP  = 0x0806,
};

//--------------------------------------------------------------------------------------------------

void mac_to_string(const Mac* mac, char* string, bool lowercase);
void string_to_mac(Mac* mac, const char* string);

void mac_send_to_ip(NetworkBuffer* buffer, u32 ip);
void mac_send_to_mac(NetworkBuffer* buffer, const Mac* destination_mac, int ether_type);
void mac_broadcast(NetworkBuffer* buffer, int ether_type);
void mac_receive();

#endif
