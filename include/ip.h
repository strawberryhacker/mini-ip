// Author: Bjørn Brodtkorb

#ifndef IP_H
#define IP_H

//--------------------------------------------------------------------------------------------------

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

enum {
    IP_PROTOCOL_ICMP = 1,
    IP_PROTOCOL_TCP  = 6,
    IP_PROTOCOL_UDP  = 17
};

//--------------------------------------------------------------------------------------------------

void ip_to_string(IpAddress ip, char* string);
IpAddress string_to_ip(const char* string);
void ip_send(NetworkBuffer* buffer, IpAddress destination_ip, int protocol);
void handle_ip_packet(NetworkBuffer* buffer);

#endif
