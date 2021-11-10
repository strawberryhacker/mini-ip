// Copyright (c) 2021 Bjørn Brodtkorb

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

Mac string_to_mac(const char* string);
void mac_to_string(const Mac* mac, char* string, bool lowercase);
void mac_send(NetworkPacket* packet, const Mac* mac, u16 ether_type);
void mac_broadcast(NetworkPacket* packet, u16 ether_type);
void mac_send_to_ip(NetworkPacket* packet, Ip ip);
void handle_mac();

#endif
