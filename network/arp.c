// Author: strawberryhacker

#include "network/arp.h"
#include "network/core.h"

//--------------------------------------------------------------------------------------------------

enum {
    ARP_ENTRY_NEW     = 0,
    ARP_ENTRY_PENDING = 1,
    ARP_ENTRY_MAPPED  = 2,
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
    
} ArpEntry;

//--------------------------------------------------------------------------------------------------

void arp_init() {

}

//--------------------------------------------------------------------------------------------------

void arp_send(NetworkBuffer* buffer, u32 destination_ip) {

}

//--------------------------------------------------------------------------------------------------

void arp_task() {

}
