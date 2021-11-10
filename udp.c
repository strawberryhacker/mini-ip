// Copyright (c) 2021 Bjørn Brodtkorb

#include "udp.h"
#include "list.h"
#include "ip.h"

//--------------------------------------------------------------------------------------------------

#define UDP_CONNECTION_COUNT 16

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    Port  source_port;
    Port  dest_port;
    u16   length;
    u16   checksum;
} UdpHeader;

//--------------------------------------------------------------------------------------------------

static UdpConnection connections[UDP_CONNECTION_COUNT];
static List free_connections;
static List used_connections;

//--------------------------------------------------------------------------------------------------

void udp_init() {
    list_init(&free_connections);
    list_init(&used_connections);

    for (int i = 0; i < UDP_CONNECTION_COUNT; i++) {
        list_init(&connections[i].packet_queue);
        list_add_first(&connections[i].list_node, &free_connections);
    }
}

//--------------------------------------------------------------------------------------------------

static u16 compute_udp_checksum(NetworkPacket* packet, Ip senders_ip, Ip target_ip) {
    u32 sum = 0;

    // IPv4 psudo header. 
    sum += (senders_ip >> 16) & 0xFFFF;
    sum += (senders_ip >> 0) & 0xFFFF;
    sum += (target_ip >> 16) & 0xFFFF;
    sum += (target_ip >> 0) & 0xFFFF;
    sum += IP_PROTOCOL_UDP;
    sum += packet->length;

    // UDP header + UDP payload.
    u16* pointer = (u16 *)&packet->data[packet->index];
    u16 length = packet->length;

    while (length > 1) {
        sum += read_be16(pointer++);
        length -= 2;
    }

    // Odd number of bytes in the payload.
    if (length) {
        sum += *(u8 *)pointer << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (u16)~sum;
}

//--------------------------------------------------------------------------------------------------

void udp_send_zero_copy(NetworkPacket* packet, Port source_port, Port dest_port, Ip ip) {
    packet->index -= sizeof(UdpHeader);
    packet->length += sizeof(UdpHeader);

    UdpHeader* header = (UdpHeader *)&packet->data[packet->index];

    write_be16(source_port, &header->source_port);
    write_be16(dest_port, &header->dest_port);
    write_be16(packet->length, &header->length);
    write_be16(0, &header->checksum);
    write_be16(compute_udp_checksum(packet, get_our_ip(), ip), &header->checksum);

    ip_send(packet, ip, IP_PROTOCOL_UDP);
}

//--------------------------------------------------------------------------------------------------

void udp_send(const void* data, int size, Port source_port, Port dest_port, Ip ip) {
    NetworkPacket* packet = allocate_network_packet();
    size = limit(size, NETWORK_PACKET_USER_SIZE);

    for (int i = 0; i < size; i++) {
        packet->data[packet->index + i] = ((u8 *)data)[i];
    }

    packet->length = size;
    udp_send_zero_copy(packet, source_port, dest_port, ip);
}

//--------------------------------------------------------------------------------------------------

void udp_listen(Port port, int max_packet_count) {
    ListNode* node = list_remove_first(&free_connections);
    UdpConnection* connection = get_struct_containing_list_node(node, UdpConnection, list_node);

    connection->max_packet_count = max_packet_count;
    connection->port = port;
    connection->packet_count = 0;

    list_add_first(node, &used_connections);
}

//--------------------------------------------------------------------------------------------------

static UdpConnection* find_connection(Port port) {
    list_iterate(it, &used_connections) {
        UdpConnection* connection = get_struct_containing_list_node(it, UdpConnection, list_node);
        if (connection->port == port) {
            return connection;
        }
    }
    
    return 0;
}

//--------------------------------------------------------------------------------------------------

NetworkPacket* udp_receive_zero_copy(Port port) {
    UdpConnection* connection = find_connection(port);

    if (connection == 0 || connection->packet_count == 0) {
        return 0;
    }

    ListNode* node = list_remove_first(&connection->packet_queue);
    NetworkPacket* packet = get_struct_containing_list_node(node, NetworkPacket, list_node);
    connection->packet_count--;

    return packet;
}

//--------------------------------------------------------------------------------------------------

int udp_receive(void* data, int size, Port port) {
    NetworkPacket* packet = udp_receive_zero_copy(port);
    
    if (packet == 0) {
        return 0;
    }

    size = limit(size, packet->length);
    
    for (int i = 0; i < size; i++) {
        ((u8 *)data)[i] = packet->data[packet->index + i];
    }

    free_network_packet(packet);
    return size;
}

//--------------------------------------------------------------------------------------------------

void handle_udp(NetworkPacket* packet) {
    if (packet->length <= sizeof(UdpHeader)) {
        free_network_packet(packet);
        return;
    }

    UdpHeader* header = (UdpHeader *)&packet->data[packet->index];

    packet->index += sizeof(UdpHeader);
    packet->length -= sizeof(UdpHeader);

    Port dest_port = read_be16(&header->dest_port);
    packet->source_port = read_be16(&header->source_port);

    UdpConnection* connection = find_connection(dest_port);
    if (connection == 0) {
        free_network_packet(packet);
        return;
    }

    // Add the UDP packet to the connection queue.
    list_add_last(&packet->list_node, &connection->packet_queue);
    connection->packet_count++;

    // Make sure the udp queue size does not get too big.
    if (connection->packet_count > connection->max_packet_count) {
        ListNode* node = list_remove_first(&connection->packet_queue);
        NetworkPacket* free_this = get_struct_containing_list_node(node, NetworkPacket, list_node);
        free_network_packet(free_this);
        connection->packet_count--;
    }
}
