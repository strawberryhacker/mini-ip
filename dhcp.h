// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef DHCP_H
#define DHCP_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void dhcp_start();
void dhcp_task();
bool dhcp_is_done();
Ip dhcp_get_server_ip();
void dhcp_release();

#endif
