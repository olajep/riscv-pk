// See LICENSE for license details.

#include <string.h>
#include "uart16550.h"
#include "fdt.h"

volatile uint8_t* uart16550;
uintptr_t uart16550_regshift;

#define UART_REG_QUEUE     0
#define UART_REG_LINESTAT  5
#define UART_REG_STATUS_RX 0x01
#define UART_REG_STATUS_TX 0x20

static inline void uart16550_out(int reg, uint8_t val)
{
  uart16550[reg << uart16550_regshift] = val;
}

static inline uint8_t uart16550_in(int reg)
{
  return uart16550[reg << uart16550_regshift];
}

void uart16550_putchar(uint8_t ch)
{
  while ((uart16550_in(UART_REG_LINESTAT) & UART_REG_STATUS_TX) == 0);
  uart16550_out(UART_REG_QUEUE, ch);
}

int uart16550_getchar()
{
  if (uart16550_in(UART_REG_LINESTAT) & UART_REG_STATUS_RX)
    return uart16550_in(UART_REG_QUEUE);
  return -1;
}

struct uart16550_scan
{
  int compat;
  uint64_t reg;
  uint32_t regshift;
  uint32_t currentspeed;
  uint32_t clockfrequency;
};

static void uart16550_open(const struct fdt_scan_node *node, void *extra)
{
  struct uart16550_scan *scan = (struct uart16550_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void uart16550_prop(const struct fdt_scan_prop *prop, void *extra)
{
  uint64_t reg;
  struct uart16550_scan *scan = (struct uart16550_scan *)extra;

  if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "ns16550a")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "xlnx,xps-uart16550-2.00.a")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &reg);
    scan->reg += reg;
  } else if (!strcmp(prop->name, "reg-offset")) {
    scan->reg += bswap(*prop->value);
  } else if (!strcmp(prop->name, "reg-shift")) {
    scan->regshift = bswap(*prop->value);
  } else if (!strcmp(prop->name, "current-speed")) {
    scan->currentspeed = bswap(*prop->value);
  } else if (!strcmp(prop->name, "clock-frequency")) {
    scan->clockfrequency = bswap(*prop->value);
  }
}

static void uart16550_done(const struct fdt_scan_node *node, void *extra)
{
  struct uart16550_scan *scan = (struct uart16550_scan *)extra;
  uint16_t divisor = 0;
  if (!scan->compat || !scan->reg || uart16550) return;

  uart16550 = (void*)(uintptr_t)scan->reg;
  uart16550_regshift = scan->regshift;
  if (scan->clockfrequency && scan->currentspeed) {
    divisor = scan->clockfrequency / (16 * scan->currentspeed);
  }

  // http://wiki.osdev.org/Serial_Ports
  uart16550_out(1, 0x00); // Disable all interrupts
  if (divisor) {
    uart16550_out(3, 0x80); // Enable DLAB (set baud rate divisor)
    uart16550_out(0, (divisor >> 0) & 0xff); // Set divisor (lo byte)
    uart16550_out(1, (divisor >> 8) & 0xff); //             (hi byte)
  }
  uart16550_out(3, 0x03); // 8 bits, no parity, one stop bit
  uart16550_out(2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
}

void query_uart16550(uintptr_t fdt)
{
  struct fdt_cb cb;
  struct uart16550_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = uart16550_open;
  cb.prop = uart16550_prop;
  cb.done = uart16550_done;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}
