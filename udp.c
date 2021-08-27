// Author: strawberryhacker

#include "udp.h"
#include "mac.h"
#include "ip.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

void handle_udp_packet(NetworkBuffer* buffer) {
    print("Got a udp packet\n");
    free_network_buffer(buffer);
}
