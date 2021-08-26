// Author: Bjørn Brodtkorb

#ifndef REGISTERS_H
#define REGISTERS_H

#include "utilities.h"

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   PER;
    __w u32   PDR;
    __r u32   PSR;
    __r u32   RESERVED0;
    __w u32   OER;
    __w u32   ODR;
    __r u32   OSR;
    __r u32   RESERVED1;
    __w u32   IFER;
    __w u32   IFDR;
    __r u32   IFSR;
    __r u32   RESERVED2;
    __w u32   SODR;
    __w u32   CODR;
    _rw u32   ODSR;
    __r u32   PDSR;
    __w u32   IER;
    __w u32   IDR;
    __r u32   IMR;
    __r u32   ISR;
    __w u32   MDER;
    __w u32   MDDR;
    __r u32   MDSR;
    __r u32   RESERVED3;
    __w u32   PUDR;
    __w u32   PUER;
    __r u32   PUSR;
    __r u32   RESERVED4;
    _rw u32   ABCDSR1;
    _rw u32   ABCDSR2;
    __r u32   RESERVED5[2];
    __w u32   IFSCDR;
    __w u32   IFSCER;
    __r u32   IFSCSR;
    _rw u32   SCDR;
    __w u32   PPDDR;
    __w u32   PPDER;
    __r u32   PPDSR;
    __r u32   RESERVED6;
    __w u32   OWER;
    __w u32   OWDR;
    __r u32   OWSR;
    __r u32   RESERVED7;
    __w u32   AIMER;
    __w u32   AIMDR;
    __r u32   AIMMR;
    __r u32   RESERVED8;
    __w u32   ESR;
    __w u32   LSR;
    __r u32   ELSR;
    __r u32   RESERVED9;
    __w u32   FELLSR;
    __w u32   REHLSR;
    __r u32   FRLHSR;
} GpioHardware;

#define GPIOA ((GpioHardware *)0x400E0E00)
#define GPIOB ((GpioHardware *)0x400E1000)
#define GPIOC ((GpioHardware *)0x400E1200)
#define GPIOD ((GpioHardware *)0x400E1400)
#define GPIOE ((GpioHardware *)0x400E1600)

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   SCER;
    __w u32   SCDR;
    __r u32   SCSR;
    __r u32   RESERVED0;
    __w u32   PCER0;
    __w u32   PCDR0;
    __r u32   PCSR0;
    __r u32   RESERVED1;
    _rw u32   MOR;
    _rw u32   MCFR;
    _rw u32   PLLAR;
    __r u32   RESERVED2;
    _rw u32   MCKR;
    __r u32   RESERVED3;
    _rw u32   USB;
    __r u32   RESERVED4;
    _rw u32   PCK0;
    _rw u32   PCK1;
    _rw u32   PCK2;
    __r u32   RESERVED5[5];
    __w u32   IER;
    __w u32   IDR;
    __r u32   SR;
    __r u32   IMR;
    _rw u32   FSMR;
    _rw u32   FSPR;
    __w u32   FOCR;
    __r u32   RESERVED6[26];
    _rw u32   WPMR;
    __r u32   WPSR;
    __r u32   RESERVED7[5];
    __w u32   PCER1;
    __w u32   PCDR1;
    __r u32   PCSR1;
} PmcHardware;

#define PMC ((PmcHardware *)0x400E0400)

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   CR;
    _rw u32   MR;
    __w u32   IER;
    __w u32   IDR;
    __r u32   IMR;
    __r u32   SR;
    __r u32   RHR;
    __w u32   THR;
    _rw u32   BRGR;
} UartHardware;

#define UART0 ((UartHardware *)0x400E0600)

//--------------------------------------------------------------------------------------------------

typedef struct {
    _rw u32   NCR;
    _rw u32   NCFGR;
    __r u32   NSR;
    _rw u32   UR;
    _rw u32   DCFGR;
    _rw u32   TSR;
    _rw u32   RBQB;
    _rw u32   TBQB;
    _rw u32   RSR;
    __r u32   ISR;
    __w u32   IER;
    __w u32   IDR;
    _rw u32   IMR;
    _rw u32   MAN;
    __r u32   RPQ;
    _rw u32   TPQ;
    __r u32   RESERVED0[16];
    _rw u32   HRB;
    _rw u32   HRT;
    _rw u32   SAB1;
    _rw u32   SAT1;
    _rw u32   SAB2;
    _rw u32   SAT2;
    _rw u32   SAB3;
    _rw u32   SAT3;
    _rw u32   SAB4;
    _rw u32   SAT4;
    _rw u32   TIDM1;
    _rw u32   TIDM2;
    _rw u32   TIDM3;
    _rw u32   TIDM4;
    __r u32   RESERVED1;
    _rw u32   IPGS;
    _rw u32   SVLAN;
    _rw u32   TPFCP;
    _rw u32   SAMB1;
    _rw u32   SAMT1;
    __r u32   RESERVED2[64];
    _rw u32   TSL;
    _rw u32   TN;
    __w u32   TA;
    _rw u32   TI;
    __r u32   EFTSL;
    __r u32   EFTN;
    __r u32   EFRSL;
    __r u32   EFRN;
    __r u32   PEFTSL;
    __r u32   PEFTN;
    __r u32   PEFRSL;
    __r u32   PEFRN;
} GmacHardware;

#define GMAC ((GmacHardware *)0x40034000)

//--------------------------------------------------------------------------------------------------

typedef struct {
    _rw u32   FMR;
    __w u32   FCR;
    __r u32   FSR;
    __r u32   FRR;
} FlashHardware;

#define FLASH ((FlashHardware *)0x400E0A00)

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   CR;
    _rw u32   MR;
    __r u32   SR;
} WdtHardware;

#define WDT ((WdtHardware *)0x400E1850)

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   CR;
    _rw u32   MMR;
    _rw u32   SMR;
    _rw u32   IADR;
    _rw u32   CWGR;
    __r u32   RESERVED0[3];
    __r u32   SR;
    __w u32   IER;
    __w u32   IDR;
    __r u32   IMR;
    __r u32   RHR;
    __w u32   THR;
} TwiHardware;

#define TWI0 ((TwiHardware *)0x400A8000)
#define TWI1 ((TwiHardware *)0x400AC000)

//--------------------------------------------------------------------------------------------------

typedef struct {
    __w u32   CCR;
    _rw u32   CMR;
    _rw u32   SMMR;
    __r u32   RAB;
    __r u32   CV;
    _rw u32   RA;
    _rw u32   RB;
    _rw u32   RC;
    __r u32   SR;
    __w u32   IER;
    __r u32   IDR;
    __r u32   IMR;
    _rw u32   EMR;
} TimerHardware;

#define TIMER0 ((TimerHardware *)0x40090000)
#define TIMER1 ((TimerHardware *)0x40090040)
#define TIMER2 ((TimerHardware *)0x40090080)
#define TIMER3 ((TimerHardware *)0x40094000)
#define TIMER4 ((TimerHardware *)0x40094040)
#define TIMER5 ((TimerHardware *)0x40094080)
#define TIMER6 ((TimerHardware *)0x40098000)
#define TIMER7 ((TimerHardware *)0x40098040)
#define TIMER8 ((TimerHardware *)0x40098080)

//--------------------------------------------------------------------------------------------------

typedef struct {
	_rw u32   ISER[8];
	__r u32   RESERVED0[24];
	_rw u32   ICER[8];
	__r u32   RESERVED1[24];
	_rw u32   ISPR[8];
	__r u32   RESERVED2[24];
	_rw u32   ICPR[8];
	__r u32   RESERVED3[24];
	_rw u32   IABR[8];
	__r u32   RESERVED4[56];
	_rw u8    IPR[240];
	__r u32   RESERVED5[644];
	__r u32   STIR;
} NvicHardware;

#define NVIC ((NvicHardware *)0xE000E100)

//--------------------------------------------------------------------------------------------------

typedef struct {
	__r u32   CPUID;
	_rw u32   ICSR;
	_rw u32   VTOR;
} ScbHardware;

#define SCB ((ScbHardware *)0xE000ED00)

#endif