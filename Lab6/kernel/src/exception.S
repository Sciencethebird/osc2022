// save general registers to stack
.macro save_reg
    sub sp, sp, 32 * 9
    stp x0, x1, [sp, 16 * 0]
    stp x2, x3, [sp, 16 * 1]
    stp x4, x5, [sp, 16 * 2]
    stp x6, x7, [sp, 16 * 3]
    stp x8, x9, [sp, 16 * 4]
    stp x10, x11, [sp, 16 * 5]
    stp x12, x13, [sp, 16 * 6]
    stp x14, x15, [sp, 16 * 7]
    stp x16, x17, [sp, 16 * 8]
    stp x18, x19, [sp, 16 * 9]
    stp x20, x21, [sp, 16 * 10]
    stp x22, x23, [sp, 16 * 11]
    stp x24, x25, [sp, 16 * 12]
    stp x26, x27, [sp, 16 * 13]
    stp x28, x29, [sp, 16 * 14]
    //str x30, [sp, 16 * 15]
    mrs x10, spsr_el1 // this is very important, I'm still not sure why
	mrs x11, elr_el1
	mrs x12, sp_el0
    stp x30, x10, [sp, 16 * 15]
    stp x11, x12, [sp, 16 * 16]
.endm

.macro load_reg
    ldp x0, x1, [sp, 16 * 0]
    ldp x2, x3, [sp, 16 * 1]
    ldp x4, x5, [sp, 16 * 2]
    ldp x6, x7, [sp, 16 * 3]
    ldp x8, x9, [sp, 16 * 4]
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    ldp x14, x15, [sp, 16 * 7]
    ldp x16, x17, [sp, 16 * 8]
    ldp x18, x19, [sp, 16 * 9]
    ldp x20, x21, [sp, 16 * 10]
    ldp x22, x23, [sp, 16 * 11]
    ldp x24, x25, [sp, 16 * 12]
    ldp x26, x27, [sp, 16 * 13]
    ldp x28, x29, [sp, 16 * 14]
    //ldr x30, [sp, 16 * 15]
    ldp x30, x10, [sp, 16 * 15]
    ldp x11, x12, [sp, 16 * 16]
    msr spsr_el1, x10
	msr elr_el1, x11
	msr sp_el0, x12
    add sp, sp, 32 * 9
.endm

// save general registers to stack
.macro save_reg_thread
    sub sp, sp, 32 * 9
    stp x0, x1, [sp, 16 * 0]
    stp x2, x3, [sp, 16 * 1]
    stp x4, x5, [sp, 16 * 2]
    stp x6, x7, [sp, 16 * 3]
    stp x8, x9, [sp, 16 * 4]
    stp x10, x11, [sp, 16 * 5]
    stp x12, x13, [sp, 16 * 6]
    stp x14, x15, [sp, 16 * 7]
    stp x16, x17, [sp, 16 * 8]
    stp x18, x19, [sp, 16 * 9]
    stp x20, x21, [sp, 16 * 10]
    stp x22, x23, [sp, 16 * 11]
    stp x24, x25, [sp, 16 * 12]
    stp x26, x27, [sp, 16 * 13]
    stp x28, x29, [sp, 16 * 14]
    //str x30, [sp, 16 * 15]
    mrs x10, tpidr_el1  // not work if you forget to store spsr
	mrs x11, elr_el1
	mrs x12, sp_el0
    stp x30, x10, [sp, 16 * 15]
    stp x11, x12, [sp, 16 * 16]
.endm

.macro load_reg_thread
    ldp x0, x1, [sp, 16 * 0]
    ldp x2, x3, [sp, 16 * 1]
    ldp x4, x5, [sp, 16 * 2]
    ldp x6, x7, [sp, 16 * 3]
    ldp x8, x9, [sp, 16 * 4]
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    ldp x14, x15, [sp, 16 * 7]
    ldp x16, x17, [sp, 16 * 8]
    ldp x18, x19, [sp, 16 * 9]
    ldp x20, x21, [sp, 16 * 10]
    ldp x22, x23, [sp, 16 * 11]
    ldp x24, x25, [sp, 16 * 12]
    ldp x26, x27, [sp, 16 * 13]
    ldp x28, x29, [sp, 16 * 14]
    //ldr x30, [sp, 16 * 15]
    ldp x30, x10, [sp, 16 * 15]
    ldp x11, x12, [sp, 16 * 16]
    msr tpidr_el1, x10
	msr elr_el1, x11
	msr sp_el0, x12
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    
    //mov x0, 0x5  10000110000000000000000000000000
    //msr spsr_el1, x0
    //ldp x0, x1, [sp, 16 * 0]
    add sp, sp, 32 * 9
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    // https://developer.arm.com/documentation/100933/0100/AArch64-exception-vector-table
    b _default_handler    // branch to a handler function.
    .align 7              // entry size is 0x80, .align will pad 0
    b _default_handler
    .align 7
    b _default_handler
    .align 7
    b _default_handler
    .align 7
    
    // EL1 -> EL1 
    b _currentEL_ELx_sync_handler // kernel exceptions
    .align 7
    b _el1_to_el1_irq_handler  // uart and EL1 timer interrupt
    .align 7
    b _default_handler
    .align 7
    b _default_handler
    .align 7

    // EL0 -> EL1
    b _lower_64_EL_sync_handler // user program
    .align 7
    b _el0_to_el1_irq_handler  // user (EL0) timer interrupt
    .align 7
    b _default_handler
    .align 7
    b _default_handler
    .align 7

    b _default_handler
    .align 7
    b _default_handler
    .align 7
    b _default_handler
    .align 7
    b _default_handler
    .align 7

_currentEL_ELx_sync_handler:
    save_reg
    mov x0, sp      // load stack pointer into function argument
    bl currentEL_ELx_sync_handler// exception.c
    load_reg
    eret            // back to original EL

_lower_64_EL_sync_handler:
    save_reg
    mov x0, sp      // load stack pointer into function argument
    bl lower_64_EL_sync_handler // exception.c
    load_reg
    eret 

_el0_to_el1_irq_handler:
    save_reg
    bl el0_to_el1_irq_handler // exception.c
    load_reg
    eret

_el1_to_el1_irq_handler:
    save_reg
    mov x0, sp 
    bl el1_to_el1_irq_handler // exception.c
    //mov sp, x0
    load_reg
    eret
    
_default_handler:
    save_reg
    bl default_handler // exception.c
    load_reg
    eret