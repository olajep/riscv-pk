// See LICENSE for license details.

#include "uart.h"

#define UART_BASE 0x60000000

#define uart_base_ptr (volatile char *)(UART_BASE)

#define UART_RX_FIFO_REG 0
#define UART_TX_FIFO_REG 0x4
#define UART_STAT_REG 0x8
#define UART_CTRL_REG 0xc

void uart_init() {
	// reset the receive FIFO and transmit FIFO
	*(uart_base_ptr + UART_CTRL_REG) = 0x3;
}

void uart_send(uint8_t data) {
	// wait until THR empty
	while((*(uart_base_ptr + UART_STAT_REG) & 0x08));
	*(uart_base_ptr + UART_TX_FIFO_REG) = data;

  if (data == '\n') {
    uart_send('\r');
  }
}

void uart_send_string(const char *str) {
	while(*str != 0) {
		uart_send(*(str++));
	}
}

void uart_send_buf(const char *buf, const int32_t len) {
	int32_t i;
	for(i=0; i<len; i++) {
		uart_send(buf[i]);
	}
}

uint8_t uart_recv() {
	// wait until RBR has data
	while(! (*(uart_base_ptr + UART_STAT_REG) & 0x01u));

	return *(uart_base_ptr + UART_RX_FIFO_REG);

}

// IRQ triggered read
uint8_t uart_read_irq() {
	return *(uart_base_ptr + UART_RX_FIFO_REG);
}

// check uart IRQ for read
uint8_t uart_check_read_irq() {
	return (*(uart_base_ptr + UART_STAT_REG) & 0x01u);
}

// enable uart read IRQ
void uart_enable_read_irq() {
	*(uart_base_ptr + UART_CTRL_REG) = 0x0010u;
}

// disable uart read IRQ
void uart_disable_read_irq() {
	*(uart_base_ptr + UART_CTRL_REG) = 0x0000u;
}
