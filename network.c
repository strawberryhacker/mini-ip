// Author: strawberryhacker

#include "network.h"
#include "arp.h"
#include "mac.h"
#include "gmac.h"
#include "udp.h"
#include "dhcp.h"

//--------------------------------------------------------------------------------------------------

#define NETWORK_PACKET_COUNT 96

//--------------------------------------------------------------------------------------------------

static NetworkPacket network_packets[NETWORK_PACKET_COUNT];
static List free_network_packets;

static Mac our_mac;
static Ip our_ip;
static Ip our_netmask;

//--------------------------------------------------------------------------------------------------

void network_init() {
    list_init(&free_network_packets);

    for (int i = 0; i < NETWORK_PACKET_COUNT; i++) {
        list_add_first(&network_packets[i].list_node, &free_network_packets);
    }

    arp_init();
    udp_init();
}

//--------------------------------------------------------------------------------------------------

NetworkPacket* allocate_network_packet() {
    ListNode* node = list_remove_first(&free_network_packets);

    // All packets have been allocated.
    if (node == 0) {
        while (1);
    }

    NetworkPacket* packet = get_struct_containing_list_node(node, NetworkPacket, list_node);
    packet->length = 0;
    packet->index = NETWORK_PACKET_HEADER_SIZE;
    
    return packet;
}

//--------------------------------------------------------------------------------------------------

void free_network_packet(NetworkPacket* packet) {
    list_add_first(&packet->list_node, &free_network_packets);
}

//--------------------------------------------------------------------------------------------------

void network_task() {
    for (int i = 0; i < RECEIVE_DESCRIPTOR_COUNT; i++) {
        NetworkPacket* packet = gmac_receive();
        if (packet == 0) {
            break;
        }

        handle_mac(packet);
    }

    arp_task();
    dhcp_task();
}

//--------------------------------------------------------------------------------------------------

void set_our_mac(const Mac* mac) {
    memory_copy(mac, &our_mac, sizeof(Mac));
    gmac_set_mac_address(mac);
}

//--------------------------------------------------------------------------------------------------

Mac* get_our_mac() {
    return &our_mac;
}

//--------------------------------------------------------------------------------------------------

void set_our_ip(Ip ip) {
    our_ip = ip;
}

//--------------------------------------------------------------------------------------------------

Ip get_our_ip() {
    return our_ip;
}

//--------------------------------------------------------------------------------------------------

void set_our_netmask(Ip netmask) {
    our_netmask = netmask;
}

//--------------------------------------------------------------------------------------------------

Ip get_our_netmask() {
    return our_netmask;
}
