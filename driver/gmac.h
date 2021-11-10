// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef GMAC_H
#define GMAC_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

#define RECEIVE_DESCRIPTOR_COUNT   32
#define TRANSMIT_DESCRIPTOR_COUNT  8

//--------------------------------------------------------------------------------------------------

void gmac_init();
void gmac_deinit();
void gmac_set_mac_address(const Mac* mac);
void gmac_send(NetworkPacket* packet);
NetworkPacket* gmac_receive();

#endif
