// Author: strawberryhacker

#include "dhcp.h"
#include "backoff.h"
#include "random.h"
#include "time.h"
#include "udp.h"
#include "ip.h"

//--------------------------------------------------------------------------------------------------

#define DHCP_START_TIMEOUT    500
#define DHCP_MAX_TIMEOUT      60000
#define DHCP_JITTER_FRACTION  4

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

//--------------------------------------------------------------------------------------------------

enum {
    DHCP_DISABLED,
    DHCP_DISCOVER,
    DHCP_REQUESTING,
    DHCP_BOUND,
    DHCP_RENEWING,
    DHCP_REBINDING,
    DHCP_ERROR,
};

enum {
    DHCP_PACKET_REQUEST,
    DHCP_PACKET_DISCOVER,
    DHCP_PACKET_RENEW,
    DHCP_PACKET_REBIND,
    DHCP_PACKET_RELEASE,
};

enum {
    DHCP_MESSAGE_TYPE_ERROR,
    DHCP_MESSAGE_TYPE_DISCOVER = 1,
    DHCP_MESSAGE_TYPE_OFFER    = 2,
    DHCP_MESSAGE_TYPE_REQUEST  = 3,
    DHCP_MESSAGE_TYPE_DECLINE  = 4,
    DHCP_MESSAGE_TYPE_ACK      = 5,
    DHCP_MESSAGE_TYPE_NACK     = 6,
    DHCP_MESSAGE_TYPE_RELEASE  = 7,
    DHCP_MESSAGE_TYPE_INFORM   = 8,
};

enum {
    DHCP_OPTION_SUBNET_MASK          = 1,
    DHCP_OPTION_REQUESTED_IP_ADDRESS = 50,
    DHCP_OPTION_LEASE_TIME           = 51,
    DHCP_OPTION_MESSAGE_TYPE         = 53,
    DHCP_OPTION_SERVER_IDENTIFIER    = 54,
    DHCP_OPTION_END                  = 255,
};

enum {
    DHCP_OPCODE_REQUEST = 1,
    DHCP_OPCODE_REPLY   = 2,
};

enum {
    OPTION_SERVER_IP    = 1 << 0,
    OPTION_OUR_IP       = 1 << 1,
    OPTION_NETMASK      = 1 << 2,
    OPTION_MESSAGE_TYPE = 1 << 3,
    OPTION_LEASE_TIME   = 1 << 4,
};

//--------------------------------------------------------------------------------------------------

typedef struct {
    int state;
    u32 transaction_id;

    Time time;
    Time lease_time;

    Ip leased_ip;
    Ip server_ip;
    Ip netmask;

    Backoff backoff;
    int aquisition_count;
} Dhcp;

typedef struct {
    int mask;

    Ip your_ip;
    Ip server_ip;
    Ip netmask;
    int message_type;
    Time lease_time;
} DhcpOptions;

typedef struct PACKED {
    u8    opcode;
    u8    hardware_type;
    u8    hardware_length;
    u8    hardware_options;
    u32   transaction_id;
    u16   seconds_elapsed;
    u16   flags;
    u32   client_ip;
    u32   your_ip;
    u32   next_server_ip;
    u32   relay_agent_ip; 
    Mac   client_mac;
    u8    padding[10];
    char  host_name[64];
    char  boot_filename[128];
    u32   magic_cookie;
} DhcpHeader;

//--------------------------------------------------------------------------------------------------

static Dhcp dhcp;
static DhcpOptions options;

//--------------------------------------------------------------------------------------------------

void dhcp_start() {
    dhcp.state = DHCP_DISCOVER;
    dhcp.transaction_id = random();
    dhcp.time = get_time();

    backoff_init(&dhcp.backoff, DHCP_START_TIMEOUT, DHCP_MAX_TIMEOUT, DHCP_JITTER_FRACTION);
    udp_listen(DHCP_CLIENT_PORT, 1);
}

//--------------------------------------------------------------------------------------------------

void add_message_type_option(int message_type, u8** data) {
    *(*data)++ = DHCP_OPTION_MESSAGE_TYPE;
    *(*data)++ = 1;
    *(*data)++ = message_type; 
}

//--------------------------------------------------------------------------------------------------

void add_requested_ip_address_option(Ip ip, u8** data) {
    *(*data)++ = DHCP_OPTION_REQUESTED_IP_ADDRESS;
    *(*data)++ = sizeof(Ip);
    write_be32(ip, *data);
    *data += sizeof(Ip);
}

//--------------------------------------------------------------------------------------------------

void add_server_identifier_option(Ip ip, u8** data) {
    *(*data)++ = DHCP_OPTION_SERVER_IDENTIFIER;
    *(*data)++ = sizeof(Ip);
    write_be32(ip, *data);
    *data += sizeof(Ip);
}

//--------------------------------------------------------------------------------------------------

void finalize_options(u8** data) {
    *(*data)++ = 255;
}

//--------------------------------------------------------------------------------------------------

static void dhcp_send_packet(int dhcp_packet_type) {
    NetworkPacket* packet = allocate_network_packet();

    DhcpHeader* header = (DhcpHeader *)&packet->data[packet->index];

    header->opcode = DHCP_OPCODE_REQUEST;
    header->hardware_type = 1;  // Ethernet.
    header->hardware_length = sizeof(Mac);
    header->hardware_options = 0;
    
    write_be32(dhcp.transaction_id, &header->transaction_id);
    write_be16(get_elapsed(dhcp.time, get_time()) / 1000, &header->seconds_elapsed);
    write_be16(0, &header->flags);
    write_be32(0, &header->client_ip);
    write_be32(0, &header->your_ip);
    write_be32(0, &header->next_server_ip);
    write_be32(0, &header->relay_agent_ip);
    write_be32(0x63825363, &header->magic_cookie);
    memory_copy(get_our_mac(), &header->client_mac, sizeof(Mac));

    u8* data = (u8 *)header + sizeof(DhcpHeader);

    // Add the options.
    if (dhcp_packet_type == DHCP_PACKET_DISCOVER) {
        add_message_type_option(DHCP_MESSAGE_TYPE_DISCOVER, &data);
    }
    else if (dhcp_packet_type == DHCP_PACKET_REQUEST) {
        add_message_type_option(DHCP_MESSAGE_TYPE_REQUEST, &data);
        add_requested_ip_address_option(dhcp.leased_ip, &data);
        add_server_identifier_option(dhcp.server_ip, &data);
    }
    else if (dhcp_packet_type == DHCP_PACKET_RENEW) {
        add_message_type_option(DHCP_MESSAGE_TYPE_REQUEST, &data);
        write_be32(dhcp.leased_ip, &header->client_ip);
    }
    else if (dhcp_packet_type == DHCP_PACKET_RELEASE) {
        add_message_type_option(DHCP_MESSAGE_TYPE_RELEASE, &data);
        add_server_identifier_option(dhcp.server_ip, &data);
        write_be32(dhcp.leased_ip, &header->client_ip);
        write_be16(0, &header->seconds_elapsed);
    }

    finalize_options(&data);
    
    packet->length = data - (u8 *)header;
    udp_send_zero_copy(packet, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, 0xFFFFFFFF);
}

//--------------------------------------------------------------------------------------------------

static bool verify_dhcp_header(DhcpHeader* header) {
    if (header->hardware_type != 1) {
        return false;
    }

    if (header->opcode != DHCP_OPCODE_REPLY) {
        return false;
    }

    if (header->hardware_length != sizeof(Mac)) {
        return false;
    }

    if (read_be32(&header->magic_cookie) != 0x63825363) {
        return false;
    }

    if (read_be32(&header->transaction_id) != dhcp.transaction_id) {
        return false;
    }

    if (memory_compare(&header->client_mac, get_our_mac(), sizeof(Mac)) == false) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

static bool dhcp_read() {
    NetworkPacket* packet = udp_receive_zero_copy(DHCP_CLIENT_PORT);
    if (packet == 0) {
        return false;
    }

    DhcpHeader* header = (DhcpHeader *)&packet->data[packet->index];

    if (verify_dhcp_header(header) == false) {
        goto return_false;
    }

    int length = packet->length - sizeof(DhcpHeader);
    u8* option_pointer = (u8 *)header + sizeof(DhcpHeader);

    options.your_ip = read_be32(&header->your_ip);
    options.mask = 0;

    // @Todo! option might be placed in the sname field. check option 52.
    while (option_pointer[0] != DHCP_OPTION_END) {
        if (length < 2) {
            goto return_false;
        }

        int type = option_pointer[0];
        int option_length = option_pointer[1];

        length -= 2;
        option_pointer += 2;

        if (option_length > length) {
            goto return_false;
        }

        if (type == DHCP_OPTION_SUBNET_MASK) {
            if (option_length != sizeof(Ip)) {
                goto return_false;
            }

            options.mask |= OPTION_NETMASK;
            options.netmask = read_be32(option_pointer);
        }
        else if (type == DHCP_OPTION_LEASE_TIME) {
            if (option_length != sizeof(u32)) {
                goto return_false;
            }

            options.mask |= OPTION_LEASE_TIME;
            options.lease_time = read_be32(option_pointer);
        }
        else if (type == DHCP_OPTION_SERVER_IDENTIFIER) {
            if (option_length != sizeof(Ip)) {
                goto return_false;
            }

            options.mask |= OPTION_SERVER_IP;
            options.server_ip = read_be32(option_pointer);
        }
        else if (type == DHCP_OPTION_MESSAGE_TYPE) {
            if (option_length != 1) {
                goto return_false;
            }

            options.mask |= OPTION_MESSAGE_TYPE;
            options.message_type = option_pointer[0];
        }

        length -= option_length;
        option_pointer += option_length;

        // Skip any padding between the options.
        while (option_pointer[0] == 0 && length) {
            length--;
            option_pointer++;
        }

        if (length == 0) {
            goto return_false;
        }
    }

    free_network_packet(packet);
    return true;

    return_false:
    free_network_packet(packet);
    return false;
}

//--------------------------------------------------------------------------------------------------

static bool try_read_offer() {
    if (dhcp_read() == false) {
        return false;
    }

    int mask = OPTION_LEASE_TIME | OPTION_SERVER_IP | OPTION_MESSAGE_TYPE;

    if ((options.mask & mask) != mask || options.message_type != DHCP_MESSAGE_TYPE_OFFER) {
        return false;
    }

    dhcp.netmask = options.netmask;
    dhcp.server_ip = options.server_ip;
    dhcp.leased_ip = options.your_ip;

    return true;
}

//--------------------------------------------------------------------------------------------------

static bool try_read_ack(bool* is_ack) {
    if (dhcp_read() == false) {
        return false;
    }

    int expected_mask = OPTION_LEASE_TIME | OPTION_MESSAGE_TYPE | OPTION_SERVER_IP;

    if ((options.mask & expected_mask) != expected_mask) {
        return false;
    }

    if (options.message_type == DHCP_MESSAGE_TYPE_NACK) {
        *is_ack = false;
        return true;
    }

    if (options.message_type != DHCP_MESSAGE_TYPE_ACK) {
        return false;
    }

    if (options.netmask != dhcp.netmask || options.server_ip != dhcp.server_ip || options.your_ip != dhcp.leased_ip) {
        return false;
    }

    *is_ack = true;
    dhcp.time = get_time();
    dhcp.lease_time = options.lease_time * 1000;

    return true;
}

//--------------------------------------------------------------------------------------------------

static bool renewing_expired() {
    return get_elapsed(dhcp.time, get_time()) > (dhcp.lease_time / 2);
}

//--------------------------------------------------------------------------------------------------

static bool rebinding_expired() {
    return get_elapsed(dhcp.time, get_time()) > (3 * dhcp.lease_time / 4);
}

//--------------------------------------------------------------------------------------------------

void dhcp_task() {
    switch (dhcp.state) {
        case DHCP_DISABLED : {
            break;
        }   
        case DHCP_DISCOVER : {
            if (backoff_timeout(&dhcp.backoff)) {
                dhcp_send_packet(DHCP_PACKET_DISCOVER);
                next_backoff(&dhcp.backoff);
            }

            if (try_read_offer()) {
                dhcp.state = DHCP_REQUESTING;
                backoff_reset(&dhcp.backoff);
            }
            break;
        }
        case DHCP_REQUESTING : {
            if (backoff_timeout(&dhcp.backoff)) {
                dhcp_send_packet(DHCP_PACKET_REQUEST);
                next_backoff(&dhcp.backoff);
            }

            bool is_ack;
            if (try_read_ack(&is_ack)) {
                if (is_ack) {
                    dhcp.state = DHCP_BOUND;

                    // Update the global network configuration.
                    set_our_ip(dhcp.leased_ip);
                    set_our_netmask(dhcp.netmask);
                }
                else {
                    dhcp.aquisition_count++;
                    dhcp.state = DHCP_DISCOVER;
                    backoff_reset(&dhcp.backoff);
                }
            }
            break;
        }
        case DHCP_BOUND : {
            if (renewing_expired()) {
                dhcp.state = DHCP_RENEWING;
                backoff_reset(&dhcp.backoff);
            }
            break;
        }
        case DHCP_RENEWING : {
            if (rebinding_expired()) {
                dhcp.state = DHCP_REBINDING;
            }

            if (backoff_timeout(&dhcp.backoff)) {
                dhcp_send_packet(DHCP_PACKET_RENEW);
                next_backoff(&dhcp.backoff);
            }

            bool is_ack;
            if (try_read_ack(&is_ack)) {
                if (is_ack) {
                    dhcp.state = DHCP_BOUND;
                }
                else {
                    dhcp.state = DHCP_DISCOVER;
                }
            }

            break;
        }
        case DHCP_REBINDING : {
            if (get_elapsed(dhcp.time, get_time()) >= dhcp.lease_time) {
                dhcp.state = DHCP_DISCOVER;
                backoff_reset(&dhcp.backoff);
            }
            break;
        }
        case DHCP_ERROR : {
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------

void dhcp_release() {
    dhcp_send_packet(DHCP_PACKET_RELEASE);
}

//--------------------------------------------------------------------------------------------------

bool dhcp_is_done() {
    return dhcp.state == DHCP_BOUND || dhcp.state == DHCP_RENEWING || dhcp.state == DHCP_REBINDING;
}

//--------------------------------------------------------------------------------------------------

Ip dhcp_get_server_ip() {
    return dhcp.server_ip;
}
