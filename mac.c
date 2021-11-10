// Author: strawberryhacker

#include "mac.h"
#include "gmac.h"
#include "arp.h"
#include "ip.h"

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    Mac target_mac;
    Mac senders_mac;
    u16 ether_type;
} MacHeader;

//--------------------------------------------------------------------------------------------------

static u8 char_to_hex(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    }

    c |= 1 << 5;  // Convert to lowercase.
    return ('a' <= c && c <= 'f') ? c - 'a' + 10 : 0;
}

//--------------------------------------------------------------------------------------------------

Mac string_to_mac(const char* string) {
    Mac mac;

    for (int i = 0; i < 6; i++) {
        if (string[0] == 0 || string[1] == 0) {
            break;
        }

        mac.address[i] = char_to_hex(string[1]) | (char_to_hex(string[0]) << 4);
        string += 2;

        if (string[0] == ':') {
            string++;
        }
    }

    return mac;
}

//--------------------------------------------------------------------------------------------------

void mac_to_string(const Mac* mac, char* string, bool lowercase) {
    static const char hex_table[] = "0123456789ABCDEF";
    char case_transform = (lowercase) ? 1 << 5 : 0;

    for (int i = 0; i < 6; i++) {
        *string++ = hex_table[(mac->address[i] >> 4) & 0xF] | case_transform;
        *string++ = hex_table[(mac->address[i] >> 0) & 0xF] | case_transform;

        if (i != 5) {
            *string++ = ':';
        }
    }

    *string = 0;
}

//--------------------------------------------------------------------------------------------------

void mac_send(NetworkPacket* packet, const Mac* mac, u16 ether_type) {
    packet->length += sizeof(MacHeader);
    packet->index -= sizeof(MacHeader);

    MacHeader* header = (MacHeader *)&packet->data[packet->index];

    memory_copy(mac, &header->target_mac, sizeof(Mac));
    memory_copy(get_our_mac(), &header->senders_mac, sizeof(Mac));
    write_be16(ether_type, &header->ether_type);
    
    gmac_send(packet);
}

//--------------------------------------------------------------------------------------------------

void mac_broadcast(NetworkPacket* packet, u16 ether_type) {
    const Mac mac = { .address = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
    mac_send(packet, &mac, ether_type);
}

//--------------------------------------------------------------------------------------------------

void mac_send_to_ip(NetworkPacket* packet, Ip ip) {
    arp_send(packet, ip);
}

//--------------------------------------------------------------------------------------------------

void handle_mac(NetworkPacket* packet) {
    if (packet->length <= sizeof(MacHeader)) {
        free_network_packet(packet);
        return;
    }

    MacHeader* header = (MacHeader *)&packet->data[packet->index];

    packet->length -= sizeof(MacHeader);
    packet->index += sizeof(MacHeader);

    u16 ether_type = read_be16(&header->ether_type);

    if (ether_type == ETHER_TYPE_ARP) {
        handle_arp(packet);
    }
    else if (ether_type == ETHER_TYPE_IPV4) {
        handle_ip(packet);
    }
    else {
        free_network_packet(packet);
    }
}
