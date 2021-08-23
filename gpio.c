// Author: strawberryhacker

#include "gpio.h"

//--------------------------------------------------------------------------------------------------

void set_pin_function(GpioHardware* gpio, int pin, int function) {
    // Re-map the pin if it's a system IO.
    if (gpio == GPIOB) {
        if (pin == 4 || pin == 5 || pin == 6 || pin == 7 || pin == 11 || pin == 12) {
            *(volatile u32 *)0x400E0314 |= 1 << pin;
        }
    }

    // Disable the GPIO controller from controlling the pin.
    gpio->PDR = 1 << pin;
    while (gpio->PSR & (1 << pin));
    
    // Set the peripheral function.
    if (function & 0b01) {
        gpio->ABCDSR1 |= 1 << pin;
    }
    else {
        gpio->ABCDSR1 &= ~(1 << pin);
    }

    if (function & 0b10) {
        gpio->ABCDSR2 |= 1 << pin;
    }
    else {
        gpio->ABCDSR2 &= ~(1 << pin);
    }
}
