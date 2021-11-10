// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef IP_H
#define IP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

enum {
    IP_PROTOCOL_UDP  = 17,
    IP_PROTOCOL_ICMP = 1,
};

//--------------------------------------------------------------------------------------------------

Ip string_to_ip(const char* string);
void ip_to_string(Ip ip, char* string);
void handle_ip(NetworkPacket* packet);
void ip_send(NetworkPacket* packet, Ip ip, int protocol);

#endif
