// Author: strawberryhacker

#include "gmac.h"
#include "registers.h"
#include "gpio.h"
#include "clock.h"
#include "print.h"

//--------------------------------------------------------------------------------------------------

enum {
    OWNER_GMAC = 0,
    OWNER_CPU  = 1,
};

#define TRANSMIT_DESCRIPTOR_COUNT 8
#define RECEIVE_DESCRIPTOR_COUNT  8

//--------------------------------------------------------------------------------------------------

typedef struct {
    union {
        u32 address_word;
        struct {
            u32 owner   : 1;
            u32 wrap    : 1;
            u32 address : 30;
        };
    };

    union {
        u32 status_word;
        struct {
            u32 length               : 13;
            u32 fcs_status           : 1;
            u32 start_of_frame       : 1;
            u32 end_of_frame         : 1;
            u32 cfi                  : 1;
            u32 vlan_priority        : 3;
            u32 priority_tag         : 1;
            u32 vlan_detected        : 1;
            u32 checksum_status      : 2;
            u32 snap_status          : 1;
            u32 address_match_status : 2;
            u32 address_match        : 1;
            u32 reserved0            : 1;
            u32 unicast_hash_match   : 1;
            u32 multicast_hash_match : 1;
            u32 broadcast_detected   : 1;
        };
    };
} RxDescriptor;

typedef struct {
    u32 address;
    union {
        u32 status_word;
        struct {
            u32 length               : 14;
            u32 reserved0            : 1;
            u32 last_buffer          : 1;
            u32 ignore_crc           : 1;
            u32 reserved1            : 3;
            u32 crc_errors           : 3;
            u32 reserved2            : 3;
            u32 late_collision       : 1;
            u32 ahb_error            : 1;
            u32 underrun             : 1;
            u32 retry_limit_exceeded : 1;
            u32 wrap                 : 1;
            u32 owner                : 1;
        };
    };
} TxDescriptor;

//--------------------------------------------------------------------------------------------------

static volatile bool link_up = false;

static volatile alignas(8) TxDescriptor gmac_tx_descriptors[TRANSMIT_DESCRIPTOR_COUNT];
static volatile alignas(8) RxDescriptor gmac_rx_descriptors[RECEIVE_DESCRIPTOR_COUNT];

static NetworkBuffer* rx_buffers[TRANSMIT_DESCRIPTOR_COUNT];
static NetworkBuffer* tx_buffers[RECEIVE_DESCRIPTOR_COUNT];

static int tx_index;
static int rx_index;

//--------------------------------------------------------------------------------------------------

void phy_write(int phy_address, int register_address, u16 data) {
    GMAC->MAN = 1 << 30 | 1 << 28 | phy_address << 23 | register_address << 18 | 1 << 17 | data;
    while ((GMAC->NSR & (1 << 2)) == 0);
}

//--------------------------------------------------------------------------------------------------

u16 phy_read(int phy_address, int register_address) {
    GMAC->MAN = 1 << 30 | 1 << 29 | phy_address << 23 | register_address << 18 | 1 << 17;
    while ((GMAC->NSR & (1 << 2)) == 0);
    return (u16)GMAC->MAN;
}

//--------------------------------------------------------------------------------------------------

// @Cleanup: Check these bits against the chip datasheet.
static void update_link_settings() {
    u16 link_status_register = phy_read(0, 5);

    bool full_speed = 0;
    bool full_duplex = 0;

    if (link_status_register & (0b11 << 7)) {
        full_speed = true;

        if (link_status_register & (1 << 8)) {
            full_duplex = true;
        }        
    }
    else if ((link_status_register & (0b11 << 5)) && (link_status_register & (1 << 6))) {
        full_duplex = true;
    }
    else {
        // @Incomplete: What happends if the code reach here?
    }

    // Update the link settings in the configuration register.
    if (full_speed) {
        print("Full speed\n");
        GMAC->NCFGR |= 1 << 0;
    }
    else {
        print("Half speed\n");
        GMAC->NCFGR &= ~(1 << 0);
    }

    if (full_duplex) {
        print("Full duplex\n");
        GMAC->NCFGR |= 1 << 1;
    }
    else {
        GMAC->NCFGR &= ~(1 << 1);
    }
}

//--------------------------------------------------------------------------------------------------

static void check_link() {
    if (phy_read(0, 1) & (1 << 2)) {
        link_up = true;
        update_link_settings();
    }
    else {
        link_up = false;
    }
}

//--------------------------------------------------------------------------------------------------

// @Incomplete: setup the PHY interrupt pin.

//--------------------------------------------------------------------------------------------------

void gmac_init() {
    set_pin_function(GPIOD,  0, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  1, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  2, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  3, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  4, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  5, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  6, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  7, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  8, PIN_FUNCTION_A);
    set_pin_function(GPIOD,  9, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 10, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 11, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 12, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 13, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 14, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 15, PIN_FUNCTION_A);
    set_pin_function(GPIOD, 16, PIN_FUNCTION_A);

    enable_peripheral_clock(44);

    // Setup the DMA descriptors.
    for (int i = 0; i < TRANSMIT_DESCRIPTOR_COUNT; i++) {
        tx_buffers[i] = allocate_network_buffer();
        gmac_tx_descriptors[i].owner = OWNER_CPU;
    }

    for (int i = 0; i < RECEIVE_DESCRIPTOR_COUNT; i++) {
        rx_buffers[i] = allocate_network_buffer();
        gmac_rx_descriptors[i].address = (u32)rx_buffers[i]->data >> 2;
    }

    gmac_tx_descriptors[TRANSMIT_DESCRIPTOR_COUNT - 1].wrap = 1;
    gmac_rx_descriptors[RECEIVE_DESCRIPTOR_COUNT - 1].wrap = 1;

    tx_index = 0;
    rx_index = 0;

    // Set the base address of the descriptor lists.
    GMAC->TBQB = (u32)gmac_tx_descriptors;
    GMAC->RBQB = (u32)gmac_rx_descriptors;

    // Set the MDIO clock speed.
    GMAC->NCFGR = 3 << 18;

    // Set MII mode.
    GMAC->UR = 1 << 0;

    // Set the receive buffer size and DMA configuration;
    GMAC->DCFGR = 4 << 0 | (NETWORK_BUFFER_SIZE / 64) << 16;

    // Enable the receiver, transmitter and the MDIO interface.
    GMAC->NCR = 1 << 2 | 1 << 3 | 1 << 4;

    // @Incomplete: Do we need to configure the capabilities first?

    // If the network cable is plugged in during startup, we don't get any interrupt from the PHY.
    // We manually check the status of the PHY at startup. 
    update_link_settings();
}

//--------------------------------------------------------------------------------------------------

bool gmac_link_is_up() {
    return link_up;
}

//--------------------------------------------------------------------------------------------------

void gmac_set_mac_address(Mac* mac) {
    GMAC->SAB1 = mac->byte[3] << 24 | mac->byte[2] << 16 | mac->byte[1] << 8 | mac->byte[0];
    GMAC->SAT1 = mac->byte[5] << 8 | mac->byte[4];
}

//--------------------------------------------------------------------------------------------------

void gmac_send(NetworkBuffer* buffer) {
    volatile TxDescriptor* descriptor = &gmac_tx_descriptors[tx_index];

    if (GMAC->TSR & (1 << 8 | 1 << 4 | 1 << 2)) {
        // @Incomplete: Handle transmit errors.
    }

    // Clear the transmit status.
    GMAC->TSR = GMAC->TSR;

    // If the network is saturated, just drop the packet.
    if (descriptor->owner == OWNER_GMAC) {
        free_network_buffer(buffer);
        return;
    }

    free_network_buffer(tx_buffers[tx_index]);
    tx_buffers[tx_index] = buffer;

    // Map in the new buffer.
    descriptor->address = (u32)&buffer->data[buffer->index];
    descriptor->length = buffer->length;
    descriptor->last_buffer = 1;
    descriptor->ignore_crc = 0;
    descriptor->owner = OWNER_GMAC;

    if (++tx_index == TRANSMIT_DESCRIPTOR_COUNT) {
        tx_index = 0;
    }

    // Start the transfer if not already started.
    GMAC->NCR |= 1 << 9;
}

//--------------------------------------------------------------------------------------------------

NetworkBuffer* gmac_receive() {
    volatile RxDescriptor* descriptor = &gmac_rx_descriptors[rx_index];

    if (descriptor->owner == OWNER_GMAC) {
        return 0;
    }

    // If the network buffer size is smaller than the MTU, the GMAC will split the packet. In this
    // case we just drop the packet.
    if (descriptor->start_of_frame == 0 || descriptor->end_of_frame == 0) {
        descriptor->owner = OWNER_GMAC;
        return 0;
    }

    NetworkBuffer* buffer = rx_buffers[rx_index];

    // Update the buffer metadata.
    buffer->length = descriptor->length;
    buffer->index = 0;
    buffer->broadcast = descriptor->broadcast_detected == 1;

    rx_buffers[rx_index] = allocate_network_buffer();

    descriptor->address = (u32)rx_buffers[rx_index]->data >> 2;
    descriptor->owner = OWNER_GMAC;

    if (++rx_index == RECEIVE_DESCRIPTOR_COUNT) {
        rx_index = 0;
    }

    return buffer;
}
