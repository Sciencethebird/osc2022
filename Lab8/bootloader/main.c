#include "mini_uart.h"
#include "utils.h"
#include "stdint.h"

union large_int_by_byte{
    uint8_t int_by_byte[8];
    uint64_t larg_int;
};

void boot_main(){
    uart_init();

    union large_int_by_byte program;
    volatile uint8_t *program_addr = (volatile uint8_t *)0x80000;

    // read dirty message until end
    while(1) {
        uint8_t data;
        data = uart_getc();
        if(data==4)break;
    }

    // read kernel size
    for(int i=0; i<8; i++)
        program.int_by_byte[i] = uart_getc();

    for(uint64_t offset=0; offset<program.larg_int; offset++)
        program_addr[offset] = uart_getc();

    
    asm volatile("br %x[program_addr]"::[program_addr]"r"(program_addr));
}