// Author: strawberryhacker

#include "time.h"
#include "registers.h"
#include "clock.h"
#include "interrupt.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

static volatile u32 time;

//--------------------------------------------------------------------------------------------------

void time_init() {
    enable_peripheral_clock(21);

    // Use internal 32kHz clock, use capture mode, RC compare resets the clock.
    TIMER0->CMR = 4 | 1 << 14;
    TIMER0->RC = 32;
    TIMER0->IER = 1 << 4;
    TIMER0->CCR = 1 << 0 | 1 << 2;

    interrupt_enable(21);
    global_interrupt_enable();
}

//--------------------------------------------------------------------------------------------------

u32 get_time() {
    return time;
}

//--------------------------------------------------------------------------------------------------

void timer0_interrupt() {
    (void)TIMER0->SR;
    time++;
}
