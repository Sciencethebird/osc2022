#pragma once

#include "gpio.h"

#define CORE0_IRQ_SOURCE ((volatile unsigned int *)(0x40000060))
#define GPU_IRQ (1 << 8)
#define CNTPNS_IRQ (1 << 1)

void sync_handler();
uint64_t el1_to_el1_irq_handler();
void el0_to_el1_irq_handler();
void default_handler();
void enable_interrupt();
void disable_interrupt();