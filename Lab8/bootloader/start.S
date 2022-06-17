#include "mm.h"

.section ".text.boot"

_start:
    // get cpu id
    mrs x1, mpidr_el1
    and x1, x1, #0xff
    // only the primary cpu (id = 0) goes to master function
    cbz x1, master
    b hang_loop

master:  // cpu id == 0
    // save dtb loading address
    ldr x1, =0x9000000
    str x0, [x1]

    // address relative
    adr     x7, _start  
    ldr     x1, =0x60000
    mov     x2, x1
    adr     x4, _end
    cmp     x7, x1
    beq     after_moved

relocation:
    cmp     x4, x7
    bls     to_target
    ldr     x3, [x7], #8
    str     x3, [x1], #8
    b       relocation

to_target:
    br x2

after_moved:
    // Clear bss
    adr     x7, bss_begin
    adr     x1, bss_end
    sub     x1, x1, x7
    bl      memzero

    // Jump to C code
    mov     sp, #LOW_MEMORY
    bl      boot_main

hang_loop:
    wfe
    b hang_loop