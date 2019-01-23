#include <string.h>
#include "uartlite.h"
#include "fdt.h"

volatile uint8_t* uartlite;

#define UART_LITE_RX_FIFO    0x0
#define UART_LITE_TX_FIFO    0x4
#define UART_LITE_STAT_REG   0x8
#define UART_LITE_CTRL_REG   0xc

#define UART_LITE_RST_FIFO   0x03
#define UART_LITE_INTR_EN    0x10
#define UART_LITE_TX_FULL    0x08
#define UART_LITE_TX_EMPTY   0x04
#define UART_LITE_RX_FULL    0x02
#define UART_LITE_RX_VALID   0x01

void uartlite_putchar(uint8_t ch) {
  if (ch == '\n') uartlite_putchar('\r');

  while (uartlite[UART_LITE_STAT_REG] & UART_LITE_TX_FULL);
  uartlite[UART_LITE_TX_FIFO] = ch;
}

int uartlite_getchar() {
  if (uartlite[UART_LITE_STAT_REG] & UART_LITE_RX_VALID)
    return uartlite[UART_LITE_RX_FIFO];
  return -1;
}

struct uartlite_scan
{
  int compat;
  uint64_t reg;
};

static void uartlite_open(const struct fdt_scan_node *node, void *extra)
{
  struct uartlite_scan *scan = (struct uartlite_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void uartlite_prop(const struct fdt_scan_prop *prop, void *extra)
{
  struct uartlite_scan *scan = (struct uartlite_scan *)extra;
  if (!strcmp(prop->name, "compatible") && !strcmp((const char*)prop->value, "xlnx,xps-uartlite-1.00.a")) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &scan->reg);
  }
}

static void uartlite_done(const struct fdt_scan_node *node, void *extra)
{
  struct uartlite_scan *scan = (struct uartlite_scan *)extra;
  if (!scan->compat || !scan->reg || uartlite) return;

  uartlite = (void*) (uintptr_t)scan->reg;

  // Enable Rx/Tx channels, Disable interrupts, Reset Rx/Tx FIFO
  uartlite[UART_LITE_CTRL_REG] = UART_LITE_RST_FIFO;
}

void query_uartlite(uintptr_t fdt)
{
  struct fdt_cb cb;
  struct uartlite_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = uartlite_open;
  cb.prop = uartlite_prop;
  cb.done = uartlite_done;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}
