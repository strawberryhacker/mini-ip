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
#include "gmac.h"
#include "network.h"

//--------------------------------------------------------------------------------------------------

void delay() {
    for (int i = 0; i < 5000000; i++) {
        __asm__("nop");
    }
}

//--------------------------------------------------------------------------------------------------

static void system_init() {
    // Disable the watchdog timer.
    // @Note: when using an external debugger the chip might reset multiple times. This is not due
    // to the watchdog timer, but is rather a debugger issue.
    WDT->MR = 1 << 15;

    clock_init();
    print_init();
}

//--------------------------------------------------------------------------------------------------

void main() {
    system_init();

    NetworkInterface interface = {
        .init    = gmac_init,
        .set_mac = gmac_set_mac_address,
        .read    = gmac_receive,
        .write   = gmac_send,
    };

    network_init(&interface, "34:34:34:34:34:34");

    while (1) {
        network_task();
    }
}
