// Author: Bjørn Brodtkorb

//--------------------------------------------------------------------------------------------------

#include "ip.h"
#include "mac.h"
#include "udp.h"

//--------------------------------------------------------------------------------------------------

enum {
    IP_FLAG_DONT_FRAGMENT  = 1 << 14,
    IP_FLAG_MORE_FRAGMENTS = 1 << 15,
};

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    u8  header_length                      : 4;
    u8  version                            : 4;
    u8  explicit_congestion_notification   : 2;
    u8  differentiated_services_code_point : 6;
    u16 length;
    u16 id;
    u16 fragment_offset;
    u8  time_to_live;
    u8  protocol;
    u16 checksum;
    u32 source_ip;
    u32 destination_ip;
} IpHeader;

//--------------------------------------------------------------------------------------------------

void ip_to_string(IpAddress ip, char* text) {
    for (u32 i = 4; i --> 0;) {
        u8 segment = (ip >> (i * 8)) & 0xFF;
        u8 base = 100;

        while (base > segment) {
            base /= 10;
        }

        if (base == 0) {
            *text++ = '0';
        }

        while (base) {
            *text++ = (segment / base) + '0';
            segment = segment % base;
            base = base / 10;
        }

        if (i) {
            *text++ = '.';
        }
    }

    *text = 0;
}

//--------------------------------------------------------------------------------------------------

IpAddress string_to_ip(const char* text) {
    IpAddress ip = 0;
    for (u32 i = 0; i < 4; i++) {
        s32 number = 0;
        s32 previous = -1;

        while (*text >= '0' && *text <= '9') {
            previous = number;
            number = number * 10 + (*text++ - '0');

            if (number == 0 && previous == 0) {
                break;
            }

            if (number > 0xFF) {
                return 0;
            }
        }

        if ((i == 3 && *text != '\0') || (i != 3 && *text != '.') || previous == -1) {
            return 0;
        }

        text++;
        ip = (ip << 8) | (number & 0xFF);
    }

    return ip;
}

//--------------------------------------------------------------------------------------------------

static u16 compute_ip_checksum(IpHeader* header, int max_length) {
    u32 sum = 0;
    int size = limit(header->header_length * 4, max_length);
    u16* pointer = (u16 *)header;

    network_write_16(0, &header->checksum);

    for (; size > 1; size -= 2) {
        sum += network_read_16(pointer++);
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (u16)~sum;
}

//--------------------------------------------------------------------------------------------------

static bool verify_ip_header(IpHeader* header, int packet_size) {
    u16 checksum = network_read_16(&header->checksum);

    if (checksum != compute_ip_checksum(header, packet_size)) {
        return false;
    }

    if ((header->header_length * 4) >= packet_size || network_read_16(&header->length) > packet_size) {
        return false;
    }
    
    if (network_read_16(&header->fragment_offset) & IP_FLAG_MORE_FRAGMENTS) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

// The IP filtering goes here.
static bool should_ip_packet_be_dropped(NetworkBuffer* buffer) {
    IpAddress our_ip = get_ip();

    // When receiving a DHCP offer the destination IP address is the offered IP address. Disable the
    // filter in that case. @Note: we should probably check the UDP port number as well.
    if (our_ip && our_ip != buffer->destination_ip) {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------

void handle_ip_packet(NetworkBuffer* buffer) {
    IpHeader* header = (IpHeader *)&buffer->data[buffer->index];

    if (verify_ip_header(header, buffer->length) == false) {
        goto free_buffer_and_return;
    }


    // The MAC frame is allways padded in order to achive a 46 byte payload. This is not detected by 
    // by the NIC hardware. 
    buffer->length = limit(buffer->length, network_read_16(&header->length));

    buffer->length -= sizeof(IpHeader);
    buffer->index += sizeof(IpHeader);

    buffer->source_ip = network_read_32(&header->source_ip);
    buffer->destination_ip = network_read_32(&header->destination_ip);

    if (should_ip_packet_be_dropped(buffer)) {
        goto free_buffer_and_return;
    }

    switch (header->protocol) {
        case IP_PROTOCOL_UDP : {
            handle_udp_packet(buffer);
            return;
        }
    }

    free_buffer_and_return:
    free_network_buffer(buffer);
}

//--------------------------------------------------------------------------------------------------

void ip_send(NetworkBuffer* buffer, IpAddress destination_ip, int protocol) {
    buffer->index -= sizeof(IpHeader);
    buffer->length += sizeof(IpHeader);

    IpHeader* header = (IpHeader *)&buffer->data[buffer->index];

    header->version                            = 4;
    header->header_length                      = sizeof(IpHeader) / 4;
    header->differentiated_services_code_point = 0;
    header->explicit_congestion_notification   = 0;
    header->protocol                           = protocol;
    header->time_to_live                       = 0xff;

    network_write_16(IP_FLAG_DONT_FRAGMENT, &header->fragment_offset);
    network_write_32(get_ip(), &header->source_ip);
    network_write_32(destination_ip, &header->destination_ip);
    network_write_16(0, &header->id);
    network_write_16(buffer->length, &header->length);
    network_write_16(0, &header->checksum);
    network_write_16(compute_ip_checksum(header, sizeof(IpHeader)), &header->checksum);
    
    mac_send_to_ip(buffer, destination_ip);
}
