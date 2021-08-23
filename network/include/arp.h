// Author: strawberryhacker

#ifndef ARP_H
#define ARP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void arp_init();
void arp_send(NetworkBuffer* buffer, u32 destination_ip);
void arp_task();

#endif
