
#include "serial.h"

#if defined(CONFIG_ENABLE_SERIAL)
// PL011 UART registers
#define UART_BASE 0x9000000
#define UART_FR 0x18          // Flag register
#define UART_FR_TXFF (1 << 5) // Transmit FIFO full
#define UART_DR 0x00          // Data register

static inline void mmio_write(uint64_t reg, uint32_t val) {
  *(volatile uint32_t *)(reg) = val;
}

static inline uint32_t mmio_read(uint64_t reg) {
  return *(volatile uint32_t *)(reg);
}

void print_char(char c) {
  while (mmio_read(UART_BASE + UART_FR) & UART_FR_TXFF)
    ;
  mmio_write(UART_BASE + UART_DR, c);
}
#else
void print_char(char c);
#endif
void init_serial() {}

void print_str(const char *str) {
  while (*str) {
    print_char(*str);
    str++;
  }
}

void print_hex(UINT8 n) {
  UINT8 c = n >> 4;
  print_char(c > 9 ? c - 10 + 'A' : c + '0');
  c = n & 0xf;
  print_char(c > 9 ? c - 10 + 'A' : c + '0');
}

void print_chars(char *c, int n) {
  for (int j = 0; j < n; j++) {
    char c1 = c[j];
    // if not printable or is \n \r, etc. special char, print '?'
    c1 = (c1 >= 32 && c1 <= 126) || c1 == '\n' || c1 == '\r' ? c1 : '?';
  }
}