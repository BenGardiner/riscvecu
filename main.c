#include <stdint.h>
#include "tiny-AES-CMAC-c/aes_cmac.h"
#include "tiny-AES-c/aes.h"

// UT32 CAN Register Offsets (SJA1000 compatible)
#define UT32CAN_MOD   0x00
#define UT32CAN_CMR   0x01
#define UT32CAN_SR    0x02
#define UT32CAN_IR    0x03
#define UT32CAN_IER   0x04
#define UT32CAN_BTR0  0x06
#define UT32CAN_BTR1  0x07
#define UT32CAN_OCR   0x08
#define UT32CAN_CDR   0x1F

// Acceptance Filter (PeliCAN Mode)
#define UT32CAN_ACR0  0x10
#define UT32CAN_ACR1  0x11
#define UT32CAN_ACR2  0x12
#define UT32CAN_ACR3  0x13
#define UT32CAN_AMR0  0x14
#define UT32CAN_AMR1  0x15
#define UT32CAN_AMR2  0x16
#define UT32CAN_AMR3  0x17

#define UT32CAN_FI    0x10  // Frame Info
#define UT32CAN_ID1   0x11
#define UT32CAN_ID2   0x12
#define UT32CAN_DATA1 0x13
#define UT32CAN_DATA2 0x14
#define UT32CAN_DATA3 0x15
#define UT32CAN_DATA4 0x16
#define UT32CAN_DATA5 0x17
#define UT32CAN_DATA6 0x18
#define UT32CAN_DATA7 0x19
#define UT32CAN_DATA8 0x1A

#define MOD_RM        1     // Reset Mode
#define CMR_TR        1     // Transmission Request
#define SR_TBS        4     // Transmit Buffer Status (Bit 2)
#define CDR_PELICAN   0x80  // PeliCAN Mode

#define CAN_BASE 0x40000000

static inline void write_can_reg8(uint32_t offset, uint8_t value) {
    *(volatile uint8_t *)(CAN_BASE + offset) = value;
}
static inline uint8_t read_can_reg8(uint32_t offset) {
    return *(volatile uint8_t *)(CAN_BASE + offset);
}

// NS16550 UART Registers
#define UART_RBR 0x00 // Receiver Buffer (Read)
#define UART_THR 0x00 // Transmitter Holding (Write)
#define UART_IER 0x04 // Interrupt Enable
#define UART_IIR 0x08 // Interrupt Identity (Read)
#define UART_FCR 0x08 // FIFO Control (Write)
#define UART_LCR 0x0C // Line Control
#define UART_MCR 0x10 // Modem Control
#define UART_LSR 0x14 // Line Status
#define UART_MSR 0x18 // Modem Status
#define UART_SCR 0x1C // Scratch

#define LSR_THRE 0x20 // Transmitter Holding Register Empty

#define UART_BASE 0xE0000000

static inline void write_uart_reg(uint32_t offset, uint8_t value) {
    *(volatile uint8_t *)(UART_BASE + offset) = value;
}
static inline uint8_t read_uart_reg(uint32_t offset) {
    return *(volatile uint8_t *)(UART_BASE + offset);
}

void delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++);
}

void uart_putc(char c) {
    // Wait for THRE (Transmitter Holding Register Empty)
    while (!(read_uart_reg(UART_LSR) & LSR_THRE));
    write_uart_reg(UART_THR, c);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

struct AES_ctx aes_ctx;
void aes_ecb_encrypt_callback(uint8_t *block) {
    AES_ECB_encrypt(&aes_ctx, block);
}

int main() {
    uint32_t counter = 0;
    uint8_t key[16] = {0xab, 0xad, 0x1d, 0xea, 0xab, 0xad, 0x1d, 0xea, 0xab, 0xad, 0x1d, 0xea, 0xab, 0xad, 0x1d, 0xea};
    uint8_t mac[16];
    uint8_t data[4];
    struct AES_CMAC_ctx cmac_ctx;

    // 1. Disable Interrupts
    write_uart_reg(UART_IER, 0x00);

    // 2. Enable FIFO (FCR)
    write_uart_reg(UART_FCR, 0x01);

    // 8 bits, no parity, 1 stop bit, 8 data bits = 0x03
    write_uart_reg(UART_LCR, 0x03);

    // 4. Modem Control (MCR) - DTR/RTS
    write_uart_reg(UART_MCR, 0x03);

    // Initialize AES context with the key
    AES_init_ctx(&aes_ctx, key);

    // Initialize CMAC context with the callback
    AES_CMAC_init_ctx(&cmac_ctx, aes_ecb_encrypt_callback);

    // 1. Enter Reset
    write_can_reg8(UT32CAN_MOD, MOD_RM);

    // 2. Configure Clock Divider
    write_can_reg8(UT32CAN_CDR, CDR_PELICAN);

    // 3. Configure Acceptance Filter (Accept All)
    // ACR = 0x00, AMR = 0xFF
    write_can_reg8(UT32CAN_ACR0, 0x00);
    write_can_reg8(UT32CAN_ACR1, 0x00);
    write_can_reg8(UT32CAN_ACR2, 0x00);
    write_can_reg8(UT32CAN_ACR3, 0x00);
    write_can_reg8(UT32CAN_AMR0, 0xFF);
    write_can_reg8(UT32CAN_AMR1, 0xFF);
    write_can_reg8(UT32CAN_AMR2, 0xFF);
    write_can_reg8(UT32CAN_AMR3, 0xFF);

    // 4. Configure Bus Timing (BTR0, BTR1)
    // 500kbps @ 24MHz (Example)
    // Renode warning: Unhandled bits in BTR1. Setting to 0 for simulation.
    write_can_reg8(UT32CAN_BTR0, 0x00);
    write_can_reg8(UT32CAN_BTR1, 0x00); // Was 0x14

    // 5. Output Control
    // Renode warning: Unhandled write to OCR. Commenting out for simulation.
    // write_can_reg8(UT32CAN_OCR, 0xAA); // Normal output

    // 6. Interrupt Enable (Disable all)
    write_can_reg8(UT32CAN_IER, 0x00);

    // 7. Leave Reset Mode
    write_can_reg8(UT32CAN_MOD, 0x00);

    while (1) {
        // Wait for Transmit Buffer to be free
        while (!(read_can_reg8(UT32CAN_SR) & SR_TBS));

        data[0] = (counter >> 24) & 0xFF;
        data[1] = (counter >> 16) & 0xFF;
        data[2] = (counter >> 8) & 0xFF;
        data[3] = counter & 0xFF;

        AES_CMAC_digest(&cmac_ctx, data, 4, mac);

        // Frame Info: Standard Frame, DLC=8
        // FF=0, RTR=0, DLC=8 -> 0x08
        write_can_reg8(UT32CAN_FI, 0x08);

        // ID: 0x123
        write_can_reg8(UT32CAN_ID1, 0x24);
        write_can_reg8(UT32CAN_ID2, 0x60);

        // Data: 32-bit timestamp + 32-bit truncated MAC
        write_can_reg8(UT32CAN_DATA1, data[0]);
        write_can_reg8(UT32CAN_DATA2, data[1]);
        write_can_reg8(UT32CAN_DATA3, data[2]);
        write_can_reg8(UT32CAN_DATA4, data[3]);

        // truncated MAC
        write_can_reg8(UT32CAN_DATA5, mac[0]);
        write_can_reg8(UT32CAN_DATA6, mac[1]);
        write_can_reg8(UT32CAN_DATA7, mac[2]);
        write_can_reg8(UT32CAN_DATA8, mac[3]);

        write_can_reg8(UT32CAN_CMR, CMR_TR);
        uart_puts("Sent frame with signature\n");

        counter++;
        delay(5000000);
    }
    return 0;
}
