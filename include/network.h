// Author: strawberryhacker

#ifndef NETWORK_H
#define NETWORK_H

#include "utilities.h"
#include "list.h"

//--------------------------------------------------------------------------------------------------

// @Note: must be a multiple of 64 bytes.
// @Note: must be bigger than 704
#define NETWORK_BUFFER_SIZE 1024

//--------------------------------------------------------------------------------------------------

typedef struct {
    volatile u8 data[NETWORK_BUFFER_SIZE];
    int length;
    int index;

    bool broadcast;

    ListNode list_node;
} NetworkBuffer;

//--------------------------------------------------------------------------------------------------

void network_buffer_init();
NetworkBuffer* allocate_network_buffer();
void free_network_buffer(NetworkBuffer* buffer);

#endif
