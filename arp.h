// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef ARP_H
#define ARP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void arp_init();
void arp_task();
void arp_send(NetworkPacket* packet, Ip ip);
void handle_arp(NetworkPacket* packet);

#endif
