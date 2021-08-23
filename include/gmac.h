// Author: strawberryhacker

#ifndef GMAC_H
#define GMAC_H

#include "utilities.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void gmac_init();
bool gmac_link_is_up();
void gmac_set_mac_address(Mac* mac);
void gmac_send(NetworkBuffer* buffer);
NetworkBuffer* gmac_receive();

#endif
