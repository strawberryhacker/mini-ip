// Author: strawberryhacker

// @Cleanup @Incomplete @Speed
// System information:
// LED           : PD22
// Button        : PA2
// PHY interrupt : PD29
// PHY reset     : PD31

#include "utilities.h"
#include "registers.h"
#include "clock.h"
#include "print.h"
#include "allocator.h"
#include "gmac.h"
#include "time.h"
#include "network.h"
#include "mac.h"

//--------------------------------------------------------------------------------------------------

u8 memory[4096];

//--------------------------------------------------------------------------------------------------

void main() {
    // Disable the watchdog timer.
    WDT->MR = 1 << 15;
    clock_init();
    print_init();
    time_init();
    allocator_set_memory_region(memory, 4096);

    NetworkInterface interface = {
        .init     = gmac_init,
        .set_mac  = gmac_set_mac_address,
        .read     = gmac_receive,
        .write    = gmac_send,
        .get_time = get_time,
    };

    network_init(&interface, "34:34:34:34:34:34");

    while (1) {
        network_task();
    }
}
