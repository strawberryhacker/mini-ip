// Author: strawberryhacker

#include "network.h"

//--------------------------------------------------------------------------------------------------

#define NETWORK_BUFFER_COUNT 50

//--------------------------------------------------------------------------------------------------

static NetworkBuffer network_buffers[NETWORK_BUFFER_COUNT];
static List network_buffer_list;
static int network_buffer_count;

//--------------------------------------------------------------------------------------------------

// Called from the GMAC init since it relies on allocating packets.
void network_buffer_init() {
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
