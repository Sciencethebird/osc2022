.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    svc 0  // calls super, generate synchronous exception (user exception)
    cmp x0, 5 // till 4
    blt 1b
1:
    b 1b