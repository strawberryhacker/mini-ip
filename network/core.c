// Author: strawberryhacker

#include "network/core.h"
#include "network/mac.h"

//--------------------------------------------------------------------------------------------------

#define NETWORK_BUFFER_COUNT 50

//--------------------------------------------------------------------------------------------------

static NetworkBuffer network_buffers[NETWORK_BUFFER_COUNT];
static List network_buffer_list;
static int network_buffer_count;

// Our network configuration. The MAC address is known at startup. The rest is resolved through DHCP.
static u32 ip;
static u32 netmask;
static u32 gateway;
static Mac mac;

NetworkInterface network_interface;

//--------------------------------------------------------------------------------------------------

static void network_buffer_init() {
    list_init(&network_buffer_list);

    for (int i = 0; i < NETWORK_BUFFER_COUNT; i++) {
        list_add_last(&network_buffers[i].list_node, &network_buffer_list);
    }
}

//--------------------------------------------------------------------------------------------------

NetworkBuffer* allocate_network_buffer() {
    ListNode* node = list_remove_first(&network_buffer_list);
    NetworkBuffer* buffer = list_get_struct(node, NetworkBuffer, list_node);

    // Adjust the cursor so that the network stack can append all necessary headers.
    buffer->length = 0;
    buffer->index = 128;

    network_buffer_count--;
    return buffer;
}

//--------------------------------------------------------------------------------------------------

void free_network_buffer(NetworkBuffer* buffer) {
    list_add_first(&buffer->list_node, &network_buffer_list);
    network_buffer_count++;
}

//--------------------------------------------------------------------------------------------------

u32 get_netmask() {
    return netmask;
}

//--------------------------------------------------------------------------------------------------

u32 get_ip() {
    return ip;
}

//--------------------------------------------------------------------------------------------------

u32 get_gateway() {
    return gateway;
}

//--------------------------------------------------------------------------------------------------

Mac* get_mac() {
    return &mac;
}

//--------------------------------------------------------------------------------------------------

void update_network_configuration(u32 new_ip, u32 new_netmask, u32 new_gateway) {
    ip = new_ip;
    netmask = new_netmask;
    gateway = new_gateway;
}

//--------------------------------------------------------------------------------------------------

void network_write_16(u16 value, void* pointer) {
    u8* source = pointer; 
    source[0] = (value >> 8) & 0xFF;
    source[1] = (value >> 0) & 0xFF;
}

//--------------------------------------------------------------------------------------------------

void network_write_32(u32 value, void* pointer) {
    u8* source = pointer;
    source[0] = (value >> 24) & 0xFF;
    source[1] = (value >> 16) & 0xFF;
    source[2] = (value >> 8 ) & 0xFF;
    source[3] = (value >> 0 ) & 0xFF;
}

//--------------------------------------------------------------------------------------------------

u16 network_read_16(void* pointer) {
    u8* source = pointer;
    return source[0] << 8 | source[1] << 0;
}

//--------------------------------------------------------------------------------------------------

u32 network_read_32(void* pointer) {
    u8* source = pointer;
    return source[0] << 24 | source[1] << 16 | source[2] << 8 | source[3] << 0;
}

//--------------------------------------------------------------------------------------------------

void network_init(NetworkInterface* interface, const char* mac_string) {
    memory_copy(interface, &network_interface, sizeof(NetworkInterface));
    string_to_mac(&mac, mac_string);

    network_buffer_init();

    // This must be called after the network_buffer_init since it relies on allocating buffers.
    network_interface.init();
    network_interface.set_mac(&mac);

    // Setup the rest of the network stack.
    print("Done setting up the network stack\n");
}

//--------------------------------------------------------------------------------------------------

void network_task() {
    mac_receive();
}
