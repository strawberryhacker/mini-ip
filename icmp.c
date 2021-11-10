// Author: strawberryhacker

#include "icmp.h"
#include "ip.h"

//--------------------------------------------------------------------------------------------------

enum {
    ICMP_TYPE_PING_REPLY   = 0,
    ICMP_TYPE_PING_REQUEST = 8,
};

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    u8 type;
    u8 code;
    u16 checksum;
    u16 id;
    u16 sequence_number;
} IcmpHeader;

//--------------------------------------------------------------------------------------------------

static u16 compute_icmp_checksum(const void* data, int size) {
    const u8* pointer = data;
    u32 sum = 0;

    while (size > 1) {
        sum += read_be16(pointer);
        pointer += 2;
        size -= 2;
    }

    if (size) {
        sum += *(u8 *)pointer << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (u16)~sum;
}

//--------------------------------------------------------------------------------------------------

void handle_icmp(NetworkPacket* packet) {
    if (packet->length < sizeof(IcmpHeader)) {
        free_network_packet(packet);
        return;
    }

    IcmpHeader* header = (IcmpHeader *)&packet->data[packet->index];

    // In case of ping we just send back the same buffer.
    if (header->type == ICMP_TYPE_PING_REQUEST) {
        header->type = ICMP_TYPE_PING_REPLY;
        header->code = 0;
        write_be16(0, &header->checksum);
        write_be16(compute_icmp_checksum(header, packet->length), &header->checksum);
        ip_send(packet, packet->senders_ip, IP_PROTOCOL_ICMP);
    }
    else {
        free_network_packet(packet);
    }
}
