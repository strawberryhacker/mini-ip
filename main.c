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
    gmac_init();
}

//--------------------------------------------------------------------------------------------------

void main() {
    system_init();

    // On-board LED.
    GPIOD->OER = 1 << 22;
    GPIOD->CODR = 1 << 22;

    print("Starting\n");

    u8 data = 1;

    while (1) {
        NetworkBuffer* buffer = allocate_network_buffer();
        for (int i = 0; i < 50; i++) {
            buffer->data[buffer->index + i] = data;
        }
        data++;
        buffer->length = 50;
        gmac_send(buffer);
        
        buffer = gmac_receive();
        if (buffer) {
            print("Got a packet : {u}\n", buffer->length);
            free_network_buffer(buffer);
        }

        delay();
        GPIOD->SODR = 1 << 22;
        delay();
        GPIOD->CODR = 1 << 22;
    }
}
