// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef NETWORK_H
#define NETWORK_H

#include "utilities.h"
#include "list.h"

//--------------------------------------------------------------------------------------------------

#define NETWORK_PACKET_SIZE         1024
#define NETWORK_PACKET_HEADER_SIZE  144
#define NETWORK_PACKET_USER_SIZE   (NETWORK_PACKET_SIZE - NETWORK_PACKET_HEADER_SIZE)

//--------------------------------------------------------------------------------------------------

typedef struct {
    u8 address[6];
} Mac;

typedef u32 Ip;
typedef u16 Port;

typedef struct {
    volatile u8 data[1024];
    int index;
    int length;

    // Set by the GMAC hardware for incoming packets.
    bool broadcast;

    Ip senders_ip;
    Ip target_ip;
    Port source_port;

    ListNode list_node;
} NetworkPacket;

//--------------------------------------------------------------------------------------------------

void network_init();
NetworkPacket* allocate_network_packet();
void free_network_packet(NetworkPacket* packet);

void network_task();

void set_our_mac(const Mac* mac);
Mac* get_our_mac();

void set_our_ip(Ip ip);
Ip get_our_ip();

void set_our_netmask(Ip netmask);
Ip get_our_netmask();

#endif
