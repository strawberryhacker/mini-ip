// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef UDP_H
#define UDP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

typedef struct {
    Port port;

    List packet_queue;
    int packet_count;
    int max_packet_count;

    ListNode list_node;
} UdpConnection;

//--------------------------------------------------------------------------------------------------

void udp_init();
void udp_send(const void* data, int size, Port source_port, Port dest_port, Ip ip);
void udp_send_zero_copy(NetworkPacket* packet, Port source_port, Port dest_port, Ip ip);
void udp_listen(Port port, int max_packet_count);
int udp_receive(void* data, int size, Port port);
NetworkPacket* udp_receive_zero_copy(Port port);
void handle_udp(NetworkPacket* packet);

#endif
