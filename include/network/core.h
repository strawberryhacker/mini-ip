// Author: strawberryhacker

#ifndef NETWORK_H
#define NETWORK_H

#include "utilities.h"
#include "list.h"

//--------------------------------------------------------------------------------------------------

#define NETWORK_BUFFER_SIZE 1024

//--------------------------------------------------------------------------------------------------

typedef struct {
    u8 byte[6];
} Mac;

typedef struct {
    volatile u8 data[NETWORK_BUFFER_SIZE];
    int length;
    int index;

    bool broadcast;

    ListNode list_node;
} NetworkBuffer;

// Used to access the hardware driver.
typedef struct {
    void (*init)();
    void (*set_mac)(Mac* mac);
    void (*write)(NetworkBuffer* buffer);
    NetworkBuffer* (*read)();
} NetworkInterface;

//--------------------------------------------------------------------------------------------------

extern NetworkInterface network_interface;

//--------------------------------------------------------------------------------------------------

void network_init(NetworkInterface* interface, const char* mac);
void network_task();

NetworkBuffer* allocate_network_buffer();
void free_network_buffer(NetworkBuffer* buffer);

u32 get_ip();
u32 get_netmask();
u32 get_gateway();
Mac* get_mac();
void update_network_configuration(u32 new_ip, u32 new_netmask, u32 new_gateway);

void network_write_16(u16 value, void* pointer);
void network_write_32(u32 value, void* pointer);
u16 network_read_16(void* pointer);
u32 network_read_32(void* pointer);

#endif
