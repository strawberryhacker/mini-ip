// Author: strawberryhacker

#include "clock.h"
#include "registers.h"

//--------------------------------------------------------------------------------------------------

void clock_init() {
    // Use 6 cycles for flash read and write operations.
    FLASH->FMR = (FLASH->FMR & ~(0xf << 8)) | (5 << 8);

    // Enable the RC oscillator.
    PMC->MOR |= (1 << 3) | 0x370000;
    while ((PMC->SR & (1 << 17)) == 0);

    // Set the RC frequency to 12 MHz.
    PMC->MOR |= (2 << 4) | 0x370000;
    while ((PMC->SR & (1 << 17)) == 0);

    // RC is the input to the main clock.
    PMC->MOR = (PMC->MOR & ~(1 << 24)) | 0x370000;
    while ((PMC->SR & (1 << 16)) == 0);

    // Start the PLL and set it to output 240 MHz.
    u32 multiplication = 20;
    u32 division       = 1;

    PMC->PLLAR = (1 << 29) | ((multiplication - 1) << 16) | (0b111111 << 8) | division;
    while ((PMC->SR & (1 << 1)) == 0);

    // Configure the master clock. The input is the PLL.
    u32 data = PMC->MCKR & ~(0b111 << 4);
    PMC->MCKR = data | (1 << 4);
    while ((PMC->SR & (1 << 3)) == 0);

    data = PMC->MCKR & ~(0b11 << 0);
    PMC->MCKR = data | (2 << 0);
    while ((PMC->SR & (1 << 3)) == 0);
}

//--------------------------------------------------------------------------------------------------

void enable_peripheral_clock(int id) {
    if (id < 32) {
        PMC->PCER0 = 1 << id;
        while ((PMC->PCSR0 & (1 << id)) == 0);
    }
    else {
        id -= 32;
        PMC->PCER1 = 1 << id;
        while ((PMC->PCSR1 & (1 << id)) == 0);
    }
}
