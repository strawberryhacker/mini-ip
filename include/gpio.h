// Author: strawberryhacker

#ifndef GPIO_H
#define GPIO_H

#include "utilities.h"
#include "registers.h"

//--------------------------------------------------------------------------------------------------

enum {
    PIN_FUNCTION_A,
    PIN_FUNCTION_B,
    PIN_FUNCTION_C,
    PIN_FUNCTION_D,
};

//--------------------------------------------------------------------------------------------------

void set_pin_function(GpioHardware* gpio, int pin, int function);

#endif
