// Copyright (c) 2021 Bjørn Brodtkorb

#include "ip.h"
#include "udp.h"
#include "mac.h"
#include "icmp.h"

//--------------------------------------------------------------------------------------------------

enum {
    IP_FLAG_DONT_FRAGMENT  = 1 << 14,
    IP_FLAG_MORE_FRAGMENTS = 1 << 15,
};

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    u8   header_length                      : 4;
    u8   version                            : 4;
    u8   explicit_congestion_notification   : 2;
    u8   differentiated_services_code_point : 6;
    u16  length;
    u16  id;
    u16  fragment_offset;
    u8   time_to_live;
    u8   protocol;
    u16  checksum;
    u32  senders_ip;
    u32  target_ip;
} IpHeader;

//--------------------------------------------------------------------------------------------------

Ip string_to_ip(const char* string) {
    Ip ip = 0;

    for (int i = 0; i < 4; i++) {
        u8 number = 0;
        
        while ('0' <= *string && *string <= '9') {
            number = number * 10 + *string - '0';
            string += 1;
        }

        ip |= number << ((3 - i) * 8);

        if (*string == '.') {
            string++;
        }
    }

    return ip;
}

//--------------------------------------------------------------------------------------------------

void ip_to_string(Ip ip, char* string) {
    for (int i = 0; i < 4; i++) {
        u8 fragment = (ip >> ((3 - i) * 8)) & 0xFF;
        int count = 0;

        char buffer[3];

        if (fragment == 0) {
            buffer[0] = '0';
            count++;
        }

        while (count < 3 && fragment) {
            buffer[count++] = fragment % 10 + '0';
            fragment /= 10;
        }

        while (count--) {
            *string++ = buffer[count];
        }

        if (i != 3) {
            *string++ = '.';
        }
    }

    *string = 0;
}

//--------------------------------------------------------------------------------------------------

static u16 compute_ip_checksum(IpHeader* header) {
    u32 sum = 0;
    int size = sizeof(IpHeader);
    u8* pointer = (u8 *)header;

    for (; size > 1; size -= 2) {
        sum += read_be16(pointer);
        pointer += 2;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (u16)~sum;
}

//--------------------------------------------------------------------------------------------------

static bool should_broadcast(Ip ip) {
    Ip inverted_netmask = ~get_our_netmask();
    return ((inverted_netmask & ip) == inverted_netmask);
}

//--------------------------------------------------------------------------------------------------

void ip_send(NetworkPacket* packet, Ip ip, int protocol) {
    packet->index -= sizeof(IpHeader);
    packet->length += sizeof(IpHeader);

    IpHeader* header = (IpHeader *)&packet->data[packet->index];

    header->version = 4;
    header->header_length = sizeof(IpHeader) / sizeof(u32);
    header->differentiated_services_code_point = 0;
    header->explicit_congestion_notification = 0;
    header->time_to_live = 0xFF;
    header->protocol = protocol;

    write_be16(packet->length, &header->length);
    write_be16(0, &header->id);
    write_be16(IP_FLAG_DONT_FRAGMENT, &header->fragment_offset);
    write_be16(0, &header->checksum);
    write_be32(get_our_ip(), &header->senders_ip);
    write_be32(ip, &header->target_ip);
    write_be16(compute_ip_checksum(header), &header->checksum);
    
    if (should_broadcast(ip)) {
        mac_broadcast(packet, ETHER_TYPE_IPV4);
    }
    else {
        mac_send_to_ip(packet, ip);
    }
}

//--------------------------------------------------------------------------------------------------

static bool verify_ip_header(IpHeader* header, int packet_size) {
    if (header->version != 4) {
        return false;
    }

    if (read_be16(&header->fragment_offset) & IP_FLAG_MORE_FRAGMENTS) {
        return false;
    }

    if ((sizeof(u32) * header->header_length) >= packet_size) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

static bool should_filter_away(NetworkPacket* packet) {
    Ip our_ip = get_our_ip();

    // @Hack: what should we do with the broadcast frames.
    if (our_ip == 0 || our_ip == packet->target_ip || packet->target_ip == 0xFFFFFFFF) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

void handle_ip(NetworkPacket* packet) {
    if (packet->length <= sizeof(IpHeader)) {
        free_network_packet(packet);
        return;
    }

    IpHeader* header = (IpHeader *)&packet->data[packet->index];

    packet->senders_ip = read_be32(&header->senders_ip);
    packet->target_ip = read_be32(&header->target_ip);

    // If the packet is too short, padding is added to reach 46 bytes in length. Because of this
    // the packet size computed by the GMAC is not always equal to the IP header packet size. 
    packet->length = limit(packet->length, read_be16(&header->length));

    if (verify_ip_header(header, packet->length) == false) {
        free_network_packet(packet);
        return;
    }

    int header_length = sizeof(u32) * header->header_length;

    packet->index += header_length;
    packet->length -= header_length;

    if (should_filter_away(packet)) {
        free_network_packet(packet);
        return;
    }

    if (header->protocol == IP_PROTOCOL_UDP) {
        handle_udp(packet);
    }
    else if (header->protocol == IP_PROTOCOL_ICMP) {
        handle_icmp(packet);
    }
    else {
        free_network_packet(packet);
    }
}
