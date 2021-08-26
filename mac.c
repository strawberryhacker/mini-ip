// Author: strawberryhacker

#include "mac.h"
#include "arp.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    Mac destination_mac;
    Mac source_mac;
    u16 type;
} MacHeader;

//--------------------------------------------------------------------------------------------------

static inline int hex_to_number(char hex) {
    if ('0' <= hex && hex <= '9') {
        return hex - '0';
    }

    // Convert to uppercase.
    hex &= ~(1 << 5);
    return ('A' <= hex && hex <= 'F') ? hex - 'A' + 10 : 0;
}

//--------------------------------------------------------------------------------------------------

// The string must be at least 18 characters long.
void mac_to_string(const Mac* mac, char* string, bool lowercase) {
    int case_mask = (lowercase) ? 1 << 5 : 0;
    char* hex_lookup = "0123456789ABCDEF";

    for (int i = 0; i < 6; i++) {
        u8 byte = mac->byte[i];

        *string++ = hex_lookup[(byte >> 4) & 0xF] | case_mask;
        *string++ = hex_lookup[(byte >> 0) & 0xF] | case_mask;

        if (i != 5) {
            *string++ = ':';
        }
    }

    *string = 0;
}

//--------------------------------------------------------------------------------------------------

// This will convert a valid MAC address into network representation. @Note: I did not bother making 
// this fault-proof. 
void string_to_mac(Mac* mac, const char* string) {
    for (int i = 0; i < 6; i++) {
        int number = 0;

        number = number * 16 + hex_to_number(*string++);
        number = number * 16 + hex_to_number(*string++);

        mac->byte[i] = (u8)number;

        if (*string == ':') {
            string++;
        }
    }
}

//--------------------------------------------------------------------------------------------------

static bool is_outgoing_broadcast(u32 ip, u32 subnet_mask) {
    return (~ip & ~subnet_mask) == 0;
}

//--------------------------------------------------------------------------------------------------

void mac_send_to_ip(NetworkBuffer* buffer, u32 ip) {
    if (is_outgoing_broadcast(ip, get_netmask())) {
        mac_broadcast(buffer, ETHER_TYPE_IPV4);
    }
    else {
        arp_send(buffer, ip);
    }
}

//--------------------------------------------------------------------------------------------------

void mac_send_to_mac(NetworkBuffer* buffer, const Mac* destination_mac, int ether_type) {
    buffer->index -= sizeof(MacHeader);
    buffer->length += sizeof(MacHeader);

    MacHeader* header = (MacHeader *)&buffer->data[buffer->index];

    memory_copy(destination_mac, &header->destination_mac, sizeof(Mac));
    memory_copy(get_mac(), &header->source_mac, sizeof(Mac));
    network_write_16(ether_type, &header->type);

    network_interface.write(buffer);
}

//--------------------------------------------------------------------------------------------------

void mac_broadcast(NetworkBuffer* buffer, int ether_type) {
    Mac broadcast = { .byte = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
    mac_send_to_mac(buffer, &broadcast, ether_type);
}

//--------------------------------------------------------------------------------------------------

void mac_receive() {
    NetworkBuffer* buffer = network_interface.read();
    if (buffer == 0) {
        return;
    }

    // This is the entry point for our network stack. Packets are read here, and handed over to the 
    // next level depending on the header. 
    if (buffer->length <= sizeof(MacHeader)) {
        free_network_buffer(buffer);
    }

    MacHeader* header = (MacHeader *)&buffer->data[buffer->index];

    buffer->index += sizeof(MacHeader);
    buffer->length -= sizeof(MacHeader);

    // @Note: In our case, the GMAC driver has hardware filtering enabled. If the driver does not support that, 
    // the broadcast flag, and the address filtering must be handled here.

    switch (network_read_16(&header->type)) {
        case ETHER_TYPE_IPV4:
            //handle_ip_packet(buffer);
            free_network_buffer(buffer);
            break;
        case ETHER_TYPE_ARP:
            handle_arp_packet(buffer);
            break;
        default:
            free_network_buffer(buffer);
            break;
    }
}
