// Author: strawberryhacker

#include "gmac.h"
#include "registers.h"
#include "stdalign.h"
#include "gpio.h"
#include "clock.h"

//--------------------------------------------------------------------------------------------------

// KSZ8081MNXIA Ethernet PHY registers.
#define PHY_REGISTER_BASIC_CONTROL                          0
#define PHY_REGISTER_BASIC_STATUS                           1
#define PHY_REGISTER_ID1                                    2
#define PHY_REGISTER_ID2                                    3
#define PHY_REGISTER_AUTO_NEGOTIATION_ADVERTISEMENT         4
#define PHY_REGISTER_AUTO_NEGOTIATION_LINK_PARTNER_ABILITY  5

//--------------------------------------------------------------------------------------------------

enum {
    OWNER_GMAC = 0,
    OWNER_CPU  = 1,
};

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

static volatile alignas(32) TxDescriptor tx_descriptors[TRANSMIT_DESCRIPTOR_COUNT];
static volatile alignas(32) RxDescriptor rx_descriptors[RECEIVE_DESCRIPTOR_COUNT];

static NetworkPacket* tx_packets[TRANSMIT_DESCRIPTOR_COUNT];
static NetworkPacket* rx_packets[RECEIVE_DESCRIPTOR_COUNT];

static int tx_index;
static int rx_index;

//--------------------------------------------------------------------------------------------------

static void phy_write_register(u8 address, u16 data) {
    GMAC->MAN = 1 << 30 | 1 << 28 | address << 18 | 1 << 17 | data;
    while ((GMAC->NSR & (1 << 2)) == 0);
}

//--------------------------------------------------------------------------------------------------

static u16 phy_read_register(u8 address) {
    GMAC->MAN = 1 << 30 | 1 << 29 | address << 18 | 1 << 17;
    while ((GMAC->NSR & (1 << 2)) == 0);
    return (u16)GMAC->MAN;
}

//--------------------------------------------------------------------------------------------------

static void update_phy_settings() {
    u16 status = phy_read_register(PHY_REGISTER_BASIC_STATUS);

    // Make sure the auto-negotiation is complete.
    if ((status & (1 << 5)) == 0) {
        return;
    }

    status = phy_read_register(PHY_REGISTER_AUTO_NEGOTIATION_LINK_PARTNER_ABILITY);

    bool full_speed = false;
    bool full_duplex = false;

    if (status & (1 << 7 | 1 << 8)) {
        full_speed = true;
        if (status & (1 << 8)) {
            full_duplex = true;
        }
    }
    else if (status & (1 << 6)) {
        full_duplex = true;
    }

    // @Verify if this is necessary. The datasheet is not very specific about it.
    GMAC->NCR &= ~(1 << 2 | 1 << 3);
    GMAC->NCFGR = (GMAC->NCFGR & ~0b11) | full_duplex << 1 | full_speed;
    GMAC->NCR |= 1 << 2 | 1 << 3;
}

//--------------------------------------------------------------------------------------------------

void gmac_init() {
    // @Verify the correct pin configuration.
    set_gpio_function(GPIOD,  0, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  1, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  2, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  3, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  4, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  5, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  6, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  7, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  8, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD,  9, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 10, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 11, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 12, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 13, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 14, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 15, GPIO_FUNCTION_A);
    set_gpio_function(GPIOD, 16, GPIO_FUNCTION_A);

    enable_peripheral_clock(44);

    // Configure the DMA descriptors.
    for (int i = 0; i < TRANSMIT_DESCRIPTOR_COUNT; i++) {
        tx_packets[i] = allocate_network_packet();
        tx_descriptors[i].owner = OWNER_CPU;
    }

    for (int i = 0; i < RECEIVE_DESCRIPTOR_COUNT; i++) {
        rx_packets[i] = allocate_network_packet();
        rx_descriptors[i].address = (u32)rx_packets[i]->data >> 2;
    }

    tx_descriptors[TRANSMIT_DESCRIPTOR_COUNT - 1].wrap = 1;
    rx_descriptors[RECEIVE_DESCRIPTOR_COUNT - 1].wrap = 1;

    tx_index = 0;
    rx_index = 0;

    GMAC->TBQB = (u32)tx_descriptors;
    GMAC->RBQB = (u32)rx_descriptors;

    // Select MII mode.
    GMAC->UR = 1 << 0;

    // DMA configuration. INCR4 AHB bursts.
    GMAC->DCFGR = 4 << 0 | (NETWORK_PACKET_SIZE / 64) << 16;

    // Copy all frames, remove frame check sequence, set MDIO clock frequency, don't copy pause frames, 
    // enable RX checksum offloading.
    GMAC->NCFGR = 1 << 17 | 4 << 18 | 1 << 23 | 1 << 24;

    // Enable the transmitter and receiver, enable the MDIO port.
    GMAC->NCR = 1 << 2 | 1 << 3 | 1 << 4;

    // @Incomplete: Handle dynamic link up and link down if some asshole unplugs the network cable.
    update_phy_settings();

    // @Bug: This prevents the first outgoing packet from being dropped. I don't know whether the
    // issue is the internal GMAC or the external Ethernet PHY.
    for (int i = 0; i < 500000; i++) {
        __asm__("nop");
    }
}

//--------------------------------------------------------------------------------------------------

void gmac_deinit() {
    GMAC->NCR = 0;
}

//--------------------------------------------------------------------------------------------------

void gmac_set_mac_address(const Mac* mac) {
    GMAC->SA[0].BOTTOM = mac->address[3] << 24 | mac->address[2] << 16 | mac->address[1] << 8 | mac->address[0];
    GMAC->SA[0].TOP = mac->address[5] << 8 | mac->address[4];
}

//--------------------------------------------------------------------------------------------------

void gmac_send(NetworkPacket* packet) {
    volatile TxDescriptor* descriptor = &tx_descriptors[tx_index];

    if (GMAC->TSR & (1 << 8 | 1 << 4 | 1 << 2)) {
        // @Incomplete: handle transmit errors.
        while (1);
    }

    // Network saturation. Drop the packet.
    if (descriptor->owner == OWNER_GMAC) {
        free_network_packet(packet);
        return;
    }

    // Free the old packet.
    free_network_packet(tx_packets[tx_index]);
    tx_packets[tx_index] = packet;

    // Link in the new packet.
    descriptor->address = (u32)&packet->data[packet->index];
    descriptor->length = packet->length;
    descriptor->last_buffer = 1;
    descriptor->ignore_crc = 0;
    descriptor->owner = OWNER_GMAC;

    if (++tx_index == TRANSMIT_DESCRIPTOR_COUNT) {
        tx_index = 0;
    }

    // Start transmission if not already started. 
    GMAC->NCR |= 1 << 9;
}

//--------------------------------------------------------------------------------------------------

NetworkPacket* gmac_receive() {
    volatile RxDescriptor* descriptor = &rx_descriptors[rx_index];

    if (descriptor->owner == OWNER_GMAC) {
        return 0;
    }

    // If the MTU is bigger than the configured NetworkPacket buffer size, the packet is split by the
    // GMAC. In this case we drop the packet.
    if (descriptor->start_of_frame == 0 || descriptor->end_of_frame == 0) {
        descriptor->owner = OWNER_GMAC;
        return 0;
    }

    NetworkPacket* packet = rx_packets[rx_index];
    
    packet->length = descriptor->length;
    packet->index = 0;
    packet->broadcast = descriptor->broadcast_detected;

    // Link in a new packet.
    rx_packets[rx_index] = allocate_network_packet();
    descriptor->address = (u32)rx_packets[rx_index]->data >> 2;
    descriptor->owner = OWNER_GMAC;

    if (++rx_index == RECEIVE_DESCRIPTOR_COUNT) {
        rx_index = 0;
    }

    return packet;
}
