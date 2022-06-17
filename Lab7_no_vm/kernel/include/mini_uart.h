#pragma once
#include <stdint.h>
#include "gpio.h"
#include "utils.h"

/* Auxilary mini UART registers */
#define AUXIRQ ((volatile unsigned int *)(MMIO_BASE + 0x00215000))
#define AUXENB ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR_REG ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT_REG ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

#define ARM_IRQ_REG_BASE (MMIO_BASE + 0x0000b000)

#define IRQ_PENDING_1 ((volatile unsigned int *)(ARM_IRQ_REG_BASE + 0x00000204))
#define ENABLE_IRQS_1 ((volatile unsigned int *)(ARM_IRQ_REG_BASE + 0x00000210))
#define DISABLE_IRQS_1 \
  ((volatile unsigned int *)(ARM_IRQ_REG_BASE + 0x0000021c))

#define AUX_IRQ (1 << 29)

#define UART_BUFFER_SIZE 2048
char read_buf[UART_BUFFER_SIZE];
char write_buf[UART_BUFFER_SIZE];
int read_buf_start, read_buf_end;
int write_buf_start, write_buf_end;

void uart_init();

// uart reading functions
char uart_getb();
char uart_getc();


// uart writing functions
void uart_send(unsigned int c);
void uart_puts(char *s);
void uart_hex(unsigned int d);

// user program function
void uart_write(const char buf[], size_t size); 
uint32_t uart_gets(char *buf, uint32_t size);
uint32_t uart_async_gets(char *buf, uint32_t size);

// uart interrupt util functions
void enable_uart_interrupt();
void disable_uart_interrupt();
void assert_transmit_interrupt();
void clear_transmit_interrupt();
void uart_handler();
char uart_async_getc();
void uart_async_puts(char *str);