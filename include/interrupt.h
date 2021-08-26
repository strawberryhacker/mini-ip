// Author: Bjørn Brodtkorb

#ifndef INTERRUPT_H
#define INTERRUPT_H

//--------------------------------------------------------------------------------------------------

#include "utilities.h"
#include "registers.h"

//--------------------------------------------------------------------------------------------------

enum {
    INTERRUPT_PRIORITY_7 = 0,
    INTERRUPT_PRIORITY_6 = 1,
    INTERRUPT_PRIORITY_5 = 2,
    INTERRUPT_PRIORITY_4 = 3,
    INTERRUPT_PRIORITY_3 = 4,
    INTERRUPT_PRIORITY_2 = 5,
    INTERRUPT_PRIORITY_1 = 6,
    INTERRUPT_PRIORITY_0 = 7,
};

//--------------------------------------------------------------------------------------------------

static inline void global_interrupt_disable() {
    __asm__("cpsid i");
}

//--------------------------------------------------------------------------------------------------

static inline void global_interrupt_enable() {
    __asm__("cpsie i");
}

//--------------------------------------------------------------------------------------------------

static inline void interrupt_enable(int interrupt_line) {
    NVIC->ISER[interrupt_line >> 5] = (1 << (interrupt_line & 0b11111));
}

//--------------------------------------------------------------------------------------------------

static inline void interrupt_disable(int interrupt_line) {
    NVIC->ICER[interrupt_line >> 5] = (1 << (interrupt_line & 0b11111));
}

//--------------------------------------------------------------------------------------------------

static inline bool interrupt_is_enabled(int interrupt_line) {
    return (NVIC->ISER[interrupt_line >> 5] & (1 << (interrupt_line & 0b11111))) != 0;
}

//--------------------------------------------------------------------------------------------------

static inline void interrupt_set_pending(int interrupt_line) {
    NVIC->ISPR[interrupt_line >> 5] = (1 << (interrupt_line & 0b11111));
}

//--------------------------------------------------------------------------------------------------

static inline void interrupt_set_priority(int interrupt_line, int priority) {
    NVIC->IPR[interrupt_line] = (u8)((priority << 5) & 0xff);
}

//--------------------------------------------------------------------------------------------------

static inline int interrupt_get_current() {
    return (int)(SCB->ICSR & 0xff);
}

#endif
