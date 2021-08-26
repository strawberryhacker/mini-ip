// Author: strawberryhacker

#include "utilities.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

#define WEAK __attribute__((weak, alias("exception_not_implemented")))

//--------------------------------------------------------------------------------------------------

extern u32 linker_relocate_start;
extern u32 linker_stack_end;
extern u32 linker_vector_start;
extern u32 linker_data_start;
extern u32 linker_data_end;
extern u32 linker_zero_start;
extern u32 linker_zero_end;

extern void main();

//--------------------------------------------------------------------------------------------------

static void reset_handler() {
    u32* source = &linker_relocate_start;
    u32* destination = &linker_data_start;

    while (destination != &linker_data_end) {
        *destination++ = *source++;
    }

    source = &linker_zero_start;

    while (source != &linker_zero_end) {
        *source++ = 0;
    }

    // @Incomplete: set the vector table base pointer.

    main();
    while (1);
}

//--------------------------------------------------------------------------------------------------

static void exception_not_implemented() {
    print("Error\n");
    while (1);
}

//--------------------------------------------------------------------------------------------------

// ARM exceptions.
void non_maskable_interrupt() WEAK;
void hard_fault_interrupt()   WEAK;
void memory_interrupt()       WEAK;
void bus_interrupt()          WEAK;
void usage_interrupt()        WEAK;
void svc_interrupt()          WEAK;
void debug_interrupt()        WEAK;
void pendsv_interrupt()       WEAK;
void systick_interrupt()      WEAK;

// Peripheral interrupts.
void sups_interrupt()         WEAK;
void rstc_interrupt()         WEAK;
void rtc_interrupt()          WEAK;
void rtt_interrupt()          WEAK;
void wdt_interrupt()          WEAK;
void pmc_interrupt()          WEAK;
void efc_interrupt()          WEAK;
void uart0_interrupt()        WEAK;
void smc_interrupt()          WEAK;
void gpioa_interrupt()        WEAK;
void gpiob_interrupt()        WEAK;
void gpioc_interrupt()        WEAK;
void gpiod_interrupt()        WEAK;
void gpioe_interrupt()        WEAK;
void usart0_interrupt()       WEAK;
void usart1_interrupt()       WEAK;
void sd_interrupt()           WEAK;
void twi0_interrupt()         WEAK;
void twi1_interrupt()         WEAK;
void spi_interrupt()          WEAK;
void dma_interrupt()          WEAK;
void timer0_interrupt()       WEAK;
void timer1_interrupt()       WEAK;
void timer2_interrupt()       WEAK;
void timer3_interrupt()       WEAK;
void timer4_interrupt()       WEAK;
void timer5_interrupt()       WEAK;
void timer6_interrupt()       WEAK;
void timer7_interrupt()       WEAK;
void timer8_interrupt()       WEAK;
void afe0_interrupt()         WEAK;
void afe1_interrupt()         WEAK;
void dac_interrupt()          WEAK;
void acc_interrupt()          WEAK;
void arm_interrupt()          WEAK;
void usb_interrupt()          WEAK;
void pwm_interrupt()          WEAK;
void can0_interrupt()         WEAK;
void can1_interrupt()         WEAK;
void aes_interrupt()          WEAK;
void nic_interrupt()          WEAK;
void uart1_interrupt()        WEAK;

//--------------------------------------------------------------------------------------------------

__attribute__ ((section(".vector_table"))) volatile const void* vectors[] = {
    &linker_stack_end,
    reset_handler,
    non_maskable_interrupt,  
    hard_fault_interrupt,    
    memory_interrupt,        
    bus_interrupt,           
    usage_interrupt,         
    0,                        
    0,                        
    0,                        
    0, 
    svc_interrupt, 
    debug_interrupt,
    0,
    pendsv_interrupt,
    systick_interrupt,
    sups_interrupt,
    rstc_interrupt,
    rtc_interrupt,
    rtt_interrupt,
    wdt_interrupt,
    pmc_interrupt,
    efc_interrupt,
    uart0_interrupt,
    smc_interrupt,
    gpioa_interrupt,
    gpiob_interrupt,
    gpioc_interrupt,
    gpiod_interrupt,
    gpioe_interrupt,
    usart0_interrupt,
    usart1_interrupt,
    sd_interrupt,
    twi0_interrupt,
    twi1_interrupt,
    spi_interrupt,
    dma_interrupt,
    timer0_interrupt,
    timer1_interrupt,
    timer2_interrupt,
    timer3_interrupt,
    timer4_interrupt,
    timer5_interrupt,
    timer6_interrupt,
    timer7_interrupt,
    timer8_interrupt,
    afe0_interrupt,
    afe1_interrupt,
    dac_interrupt,
    acc_interrupt,
    arm_interrupt,
    usb_interrupt,
    pwm_interrupt,
    can0_interrupt,
    can1_interrupt,
    aes_interrupt,
    0,
    0,
    0,
    0,
    nic_interrupt,
    uart1_interrupt
};
