// Copyright (c) 2021 Bjørn Brodtkorb

#include "arp.h"
#include "network.h"
#include "list.h"
#include "time.h"
#include "mac.h"
#include "ip.h"

//--------------------------------------------------------------------------------------------------

#define ARP_ENTRY_COUNT                8
#define ARP_ENTRY_MAX_QUEUE_SIZE       2
#define ARP_ENTRY_EXPIRATION_INTERVAL  60000

#define ARP_RETRY_INTERVAL   1000
#define ARP_RETRY_MAX_COUNT  3

#define HARDWARE_TYPE_ETHERNET  1

//--------------------------------------------------------------------------------------------------

enum {
    ARP_TYPE_GRATUITOUS,
    ARP_TYPE_PROBE,
    ARP_TYPE_ANNOUNCEMENT,
    ARP_TYPE_REQUEST,
    ARP_TYPE_REPLY,
};

enum {
    ARP_OPERATION_REQUEST = 1,
    ARP_OPERATION_REPLY   = 2,
};

//--------------------------------------------------------------------------------------------------

typedef struct PACKED {
    u16 hardware_type;
    u16 protocol_type;
    u8  hardware_length;
    u8  protocol_length;
    u16 operation;
    Mac senders_mac;
    Ip  senders_ip;
    Mac target_mac;
    Ip  target_ip;
} ArpHeader;

typedef struct {
    Mac      mac;
    Ip       ip;
    bool     contain_valid_mapping;
    Time     time;
    int      retry_count;
    List     packet_queue;
    int      packet_count;
    ListNode list_node;
} ArpEntry;

//--------------------------------------------------------------------------------------------------

static ArpEntry arp_entries[ARP_ENTRY_COUNT];
static List free_entries;
static List used_entries;

//--------------------------------------------------------------------------------------------------

void arp_init() {
    list_init(&free_entries);
    list_init(&used_entries);

    for (int i = 0; i < ARP_ENTRY_COUNT; i++) {
        list_init(&arp_entries[i].packet_queue);
        list_add_first(&arp_entries[i].list_node, &free_entries);
    }
}

//--------------------------------------------------------------------------------------------------

static void free_entry(ArpEntry* entry) {
    // Delete any pending packets before freing the entry.
    while (1) {
        ListNode* node = list_remove_first(&entry->packet_queue);

        if (node == 0) {
            break;
        }

        NetworkPacket* packet = get_struct_containing_list_node(node, NetworkPacket, list_node);
        free_network_packet(packet);
        entry->packet_count--;
    }

    list_remove(&entry->list_node);
    list_add_first(&entry->list_node, &free_entries);
}

//--------------------------------------------------------------------------------------------------

static ArpEntry* allocate_entry() {
    ListNode* node = list_remove_first(&free_entries);

    // Handle cases where no free ARP entries are avaliable.
    if (node == 0) {
        ListNode* used_node = list_get_first(&used_entries);
        ArpEntry* entry = get_struct_containing_list_node(used_node, ArpEntry, list_node);

        free_entry(entry);
        node = &entry->list_node;
    }

    ArpEntry* entry = get_struct_containing_list_node(node, ArpEntry, list_node);

    entry->contain_valid_mapping = false;
    entry->retry_count = 0;
    entry->packet_count = 0;

    list_add_last(&entry->list_node, &used_entries);
    return entry;
}

//--------------------------------------------------------------------------------------------------

// The target_mac is only needed for ARP reply. The target_ip is not needed for ARP announcement or 
// gratuitous ARP. 
static void send_arp_packet(Mac* target_mac, Ip target_ip, int arp_type) {
    NetworkPacket* packet = allocate_network_packet();

    ArpHeader* header = (ArpHeader *)&packet->data[packet->index];
    packet->length = sizeof(ArpHeader);

    if (arp_type != ARP_TYPE_REPLY) {
        target_mac = &(Mac){ .address = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };
    }

    if (arp_type == ARP_TYPE_ANNOUNCEMENT || arp_type == ARP_TYPE_GRATUITOUS) {
        target_ip = get_our_ip();
    }

    Ip senders_ip = (arp_type == ARP_TYPE_PROBE) ? 0 : get_our_ip();
    u16 operation = (arp_type == ARP_TYPE_GRATUITOUS || arp_type == ARP_TYPE_REPLY) ? ARP_OPERATION_REPLY : ARP_OPERATION_REQUEST;

    memory_copy(get_our_mac(), &header->senders_mac, sizeof(Mac));
    memory_copy(target_mac, &header->target_mac, sizeof(Mac));

    header->hardware_length = sizeof(Mac);
    header->protocol_length = sizeof(Ip);

    write_be16(HARDWARE_TYPE_ETHERNET, &header->hardware_type);
    write_be16(ETHER_TYPE_IPV4, &header->protocol_type);
    write_be16(operation, &header->operation);
    write_be32(senders_ip, &header->senders_ip);
    write_be32(target_ip, &header->target_ip);

    if (arp_type == ARP_TYPE_REPLY) {
        mac_send(packet, target_mac, ETHER_TYPE_ARP);
    }
    else {
        mac_broadcast(packet, ETHER_TYPE_ARP);
    }
}

//--------------------------------------------------------------------------------------------------

static ArpEntry* find_arp_entry(Ip ip) {
    list_iterate(it, &used_entries) {
        ArpEntry* entry = get_struct_containing_list_node(it, ArpEntry, list_node);

        if (entry->ip == ip) {
            return entry;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------

static void add_to_arp_entry_queue(NetworkPacket* packet, ArpEntry* entry) {
    if (entry->packet_count == ARP_ENTRY_MAX_QUEUE_SIZE) {
        ListNode* node = list_remove_first(&entry->packet_queue);
        NetworkPacket* packet_to_delete = get_struct_containing_list_node(node, NetworkPacket, list_node);

        // Evict the least recently added packet.
        free_network_packet(packet_to_delete);
        entry->packet_count--;
    }

    list_add_last(&packet->list_node, &entry->packet_queue);
    entry->packet_count++;
}

//--------------------------------------------------------------------------------------------------

void arp_send(NetworkPacket* packet, Ip ip) {
    ArpEntry* entry = find_arp_entry(ip);

    if (entry) {
        if (entry->contain_valid_mapping) {
            mac_send(packet, &entry->mac, ETHER_TYPE_IPV4);
            return;
        }
        
        add_to_arp_entry_queue(packet, entry);
        return;
    }

    entry = allocate_entry();

    entry->ip = ip;
    entry->contain_valid_mapping = false;
    entry->time = get_time();

    add_to_arp_entry_queue(packet, entry);
    send_arp_packet(0, ip, ARP_TYPE_REQUEST);
}

//--------------------------------------------------------------------------------------------------

static bool validate_arp_header(ArpHeader* header) {
    if (read_be16(&header->hardware_type) != HARDWARE_TYPE_ETHERNET) {
        return false;
    }

    if (read_be16(&header->protocol_type) != ETHER_TYPE_IPV4) {
        return false;
    }

    if (header->hardware_length != sizeof(Mac) || header->protocol_length != sizeof(Ip)) {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

static void send_packets_on_entry(ArpEntry* entry) {
    while (1) {
        ListNode* node = list_remove_first(&entry->packet_queue);

        if (node == 0) {
            return;
        }

        NetworkPacket* packet = get_struct_containing_list_node(node, NetworkPacket, list_node);
        mac_send(packet, &entry->mac, ETHER_TYPE_IPV4);
    }
}

//--------------------------------------------------------------------------------------------------

static void update_arp_mapping(Ip ip, Mac* mac, bool update_only_if_not_valid) {
    ArpEntry* entry = find_arp_entry(ip);

    if (entry == 0 || (update_only_if_not_valid && entry->contain_valid_mapping == true)) {
        return;
    }

    memory_copy(mac, &entry->mac, sizeof(Mac));
    entry->contain_valid_mapping = true;
    entry->time = get_time();
    send_packets_on_entry(entry);
}

//--------------------------------------------------------------------------------------------------

void handle_arp(NetworkPacket* packet) {
    if (packet->length < sizeof(ArpHeader)) {
        goto free;
    }
    
    ArpHeader* header = (ArpHeader *)&packet->data[packet->index];

    if (validate_arp_header(header) == false) {
        goto free;
    }

    u16 operation = read_be16(&header->operation);
    Ip senders_ip = read_be32(&header->senders_ip);
    Ip target_ip = read_be32(&header->target_ip);

    if (operation == ARP_OPERATION_REPLY) {
        if (senders_ip != target_ip && target_ip == get_our_ip()) {
            // Incoming ARP reply. 
            update_arp_mapping(senders_ip, &header->senders_mac, true);
        }
    }
    else if (operation == ARP_OPERATION_REQUEST) {
        if (senders_ip && senders_ip != target_ip && target_ip == get_our_ip()) {
            // Incoming ARP request. Respond with ARP reply.
            send_arp_packet(&header->senders_mac, header->senders_ip, ARP_TYPE_REPLY);
        }
    }

    free:
    free_network_packet(packet);
}

//--------------------------------------------------------------------------------------------------

void arp_task() {
    list_iterate_safe(it, &used_entries) {
        ArpEntry* entry = get_struct_containing_list_node(it, ArpEntry, list_node);


        if (entry->contain_valid_mapping) {
            if (get_elapsed(entry->time, get_time()) > ARP_ENTRY_EXPIRATION_INTERVAL) {
                free_entry(entry);
            }
        }
        else if (get_elapsed(entry->time, get_time()) > ARP_RETRY_INTERVAL) {
            if (entry->retry_count < ARP_RETRY_MAX_COUNT) {
                send_arp_packet(0, entry->ip, ARP_TYPE_REQUEST);
                entry->retry_count++;
                entry->time = get_time();
            }
            else {
                free_entry(entry);
            }
        }
    }
}
