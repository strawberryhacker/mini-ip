// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef ICMP_H
#define ICMP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void handle_icmp(NetworkPacket* packet);

#endif
