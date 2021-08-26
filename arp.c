// Author: strawberryhacker

#include "arp.h"
#include "network.h"
#include "mac.h"
#include "array.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

#define ARP_MAX_ENTRY_COUNT            16
#define ARP_MAX_RETRY_COUNT            1000
#define ARP_RETRY_INTERVAL             400
#define ARP_ENTRY_TIMEOUT_INTERVAL     3600000
#define ARP_MAX_OUTPUT_QUEUE_SIZE      1
#define ENABLE_GRATUITOUS_ARP_HANDLING true

enum {
    ARP_STATE_PENDING = 0,
    ARP_STATE_MAPPED  = 1,
};

enum {
    ARP_OPERATION_REQUEST = 1,
    ARP_OPERATION_REPLY   = 2,
};

enum {
    ARP_TYPE_REQUEST      = 0,
    ARP_TYPE_PROBE        = 1,
    ARP_TYPE_ANNOUNCEMENT = 2,
    ARP_TYPE_GRATUITOUS   = 3,
    ARP_TYPE_REPLY        = 4,
};

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    u16 hardware_type;
    u16 protocol_type;
    u8 mac_length;
    u8 ip_length;
    u16 operation;
    Mac source_mac;
    u32 source_ip;
    Mac destination_mac;
    u32 destination_ip;
} ArpHeader;

typedef struct {
    int state;

    u32 ip;
    Mac mac;

    List output_queue;
    int output_queue_size;

    int retry_count;
    u32 time;
} ArpEntry;

define_array(arp_table, ArpTable, ArpEntry);

//--------------------------------------------------------------------------------------------------

static ArpTable arp_table;

//--------------------------------------------------------------------------------------------------

void arp_init() {
    arp_table_init(&arp_table, ARP_MAX_ENTRY_COUNT);
}

//--------------------------------------------------------------------------------------------------

static int find_entry(IpAddress ip) {
    for (int i = 0; i < arp_table.count; i++) {
        if (arp_table.items[i].ip == ip) {
            return i;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------

static void arp_entry_send_queued_packets(int index) {
    ArpEntry* entry = &arp_table.items[index];

    while (1) {
        ListNode* node = list_remove_first(&entry->output_queue);
        if (node == 0) {
            return;
        }

        NetworkBuffer* buffer = list_get_struct(node, NetworkBuffer, list_node);
        mac_send_to_mac(buffer, &entry->mac, ETHER_TYPE_IPV4);
    }
}

//--------------------------------------------------------------------------------------------------

static void delete_arp_entry(int index) {
    ArpEntry* entry = &arp_table.items[index];

    while (1) {
        ListNode* node = list_remove_first(&entry->output_queue);
        if (node == 0) {
            break;
        }

        NetworkBuffer* buffer = list_get_struct(node, NetworkBuffer, list_node);
        free_network_buffer(buffer);
    }

    arp_table_remove(&arp_table, index);
}

//--------------------------------------------------------------------------------------------------

void add_packet_to_arp_queue(NetworkBuffer* buffer, int index) {
    ArpEntry* entry = &arp_table.items[index];

    // Keep the output queue below a certain limit.
    if (entry->output_queue_size >= ARP_MAX_OUTPUT_QUEUE_SIZE) {
        NetworkBuffer* delete_this = list_get_struct(list_remove_first(&entry->output_queue), NetworkBuffer, list_node);
        free_network_buffer(delete_this);
        entry->output_queue_size--;
    }

    list_add_last(&buffer->list_node, &entry->output_queue);
    entry->output_queue_size++;
}

//--------------------------------------------------------------------------------------------------

static void send_arp_packet(u32 destination_ip, const Mac* destination_mac, int arp_type) {
    NetworkBuffer* buffer = allocate_network_buffer();

    buffer->index -= sizeof(ArpHeader);
    buffer->length += sizeof(ArpHeader);

    ArpHeader* header = (ArpHeader *)&buffer->data[buffer->index];

    network_write_16(1, &header->hardware_type);
    network_write_16(ETHER_TYPE_IPV4, &header->protocol_type);
    header->ip_length = 4;
    header->mac_length = 6;

    u32 source_ip = get_ip();
    Mac* source_mac = get_mac();
    u16 operation = ARP_OPERATION_REQUEST;

    if (arp_type != ARP_TYPE_REPLY) {
        destination_mac = &(Mac){ .byte = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
    }

    if (arp_type == ARP_TYPE_PROBE) {
        source_ip = 0;
    }
    else if (arp_type == ARP_TYPE_ANNOUNCEMENT) {
        source_ip = destination_ip;
    }
    else if (arp_type == ARP_TYPE_GRATUITOUS) {
        operation = ARP_OPERATION_REPLY;
        destination_ip = source_ip;
    }
    else if (arp_type == ARP_TYPE_REPLY) {
        operation = ARP_OPERATION_REPLY;
    }

    memory_copy(source_mac, &header->source_mac, sizeof(Mac));
    memory_copy(destination_mac, &header->destination_mac, sizeof(Mac));
    network_write_32(source_ip, &header->source_ip);
    network_write_32(destination_ip, &header->destination_ip);
    network_write_16(operation, &header->operation);

    if (arp_type == ARP_TYPE_REPLY) {
        mac_send_to_mac(buffer, destination_mac, ETHER_TYPE_ARP);
    }
    else {
        mac_broadcast(buffer, ETHER_TYPE_ARP);
    }
}

//--------------------------------------------------------------------------------------------------

static void send_arp_request(u32 destination_ip) {
    send_arp_packet(destination_ip, 0, ARP_TYPE_REQUEST);
}

//--------------------------------------------------------------------------------------------------

static void send_arp_probe(u32 destination_ip) {
    send_arp_packet(destination_ip, 0, ARP_TYPE_PROBE);
}

//--------------------------------------------------------------------------------------------------

static void send_arp_announcement(u32 destination_ip) {
    send_arp_packet(destination_ip, 0, ARP_TYPE_ANNOUNCEMENT);
}

//--------------------------------------------------------------------------------------------------

static void send_arp_reply(u32 destination_ip, const Mac* destination_mac) {
    send_arp_packet(destination_ip, destination_mac, ARP_TYPE_REPLY);
}

//--------------------------------------------------------------------------------------------------

void arp_send(NetworkBuffer* buffer, u32 destination_ip) {
    // Do a linear search through the ARP table. Normally we find an entry with matching IP address
    // which is in the MAPPED state. It does not make sense to implement any search algorithm here.
    int index = find_entry(destination_ip);

    if (index >= 0 && arp_table.items[index].ip == destination_ip) {
        if (arp_table.items[index].state == ARP_STATE_MAPPED) {
            mac_send_to_mac(buffer, &arp_table.items[index].mac, ETHER_TYPE_IPV4);
        }
        else {
            add_packet_to_arp_queue(buffer, index);
        }

        return;
    }

    if (arp_table.count >= ARP_MAX_ENTRY_COUNT) {
        delete_arp_entry(0);
    }

    // Make a new ARP entry. Mark it as pending. Add the packet to the output queue.
    index = arp_table_append_nocopy(&arp_table);
    ArpEntry* entry = &arp_table.items[index];

    entry->ip = destination_ip;
    entry->retry_count = 1;
    entry->state = ARP_STATE_PENDING;
    entry->output_queue_size = 0;
    list_init(&entry->output_queue);
    add_packet_to_arp_queue(buffer, index);

    entry->time = network_interface.get_time();
    send_arp_request(destination_ip);
}

//--------------------------------------------------------------------------------------------------

void arp_task() {
    for (int i = 0; i < arp_table.count;) {
        ArpEntry* entry = &arp_table.items[i];

        if (entry->state == ARP_STATE_PENDING && time_difference(entry->time, network_interface.get_time()) > ARP_RETRY_INTERVAL) {
            if (entry->retry_count == ARP_MAX_RETRY_COUNT) {
                delete_arp_entry(i);
                continue;
            }
            else {
                entry->time = network_interface.get_time();
                send_arp_request(entry->ip);
                entry->retry_count++;
            }
        }
        else if (entry->state == ARP_STATE_MAPPED && time_difference(entry->time, network_interface.get_time()) > ARP_ENTRY_TIMEOUT_INTERVAL) {
            // We could set the state to PENDING and acquire the MAC address again. I do not know what is reccomended.
            delete_arp_entry(i);
            continue;
        }

        i++;
    }
}

//--------------------------------------------------------------------------------------------------

static bool verify_arp_header(ArpHeader* header) {
    if (network_read_16(&header->hardware_type) != 1) {
        return false;
    }

    if (network_read_16(&header->protocol_type) != ETHER_TYPE_IPV4) {
        return false;
    }

    if (header->ip_length != 4 || header->mac_length != 6) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

void handle_arp_packet(NetworkBuffer* buffer) {
    ArpHeader* header = (ArpHeader *)&buffer->data[buffer->index];

    if (verify_arp_header(header) == false) {
        goto delete_and_return;
    }

    int operation = network_read_16(&header->operation);
    IpAddress source_ip = network_read_32(&header->source_ip);
    IpAddress destination_ip = network_read_32(&header->destination_ip);

    if (destination_ip != get_ip()) {
        goto delete_and_return;
    }

    // @Incomplete: add ARP probe handling.
    if (source_ip == destination_ip) {
        if (ENABLE_GRATUITOUS_ARP_HANDLING && operation == ARP_OPERATION_REPLY) {
            int index = find_entry(source_ip);
            if (index >= 0 && arp_table.items[index].state == ARP_STATE_MAPPED) {
                memory_copy(&header->source_mac, &arp_table.items[index].mac, sizeof(MacAddress));
            }
        }
    }
    else if (operation == ARP_OPERATION_REQUEST) {
        send_arp_reply(source_ip, &header->source_mac);
    }
    else if (operation == ARP_OPERATION_REPLY && buffer->broadcast == 0) {
        int index = find_entry(source_ip);
        if (index >= 0 && arp_table.items[index].state == ARP_STATE_PENDING) {
            memory_copy(&header->source_mac, &arp_table.items[index].mac, sizeof(MacAddress));
            arp_entry_send_queued_packets(index);
        }
    }

    delete_and_return:
    free_network_buffer(buffer);
}
