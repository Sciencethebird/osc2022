.section ".text.boot"

.global _start

_start:
    bl main
    b exit

.section ".text"

.global getpid
.global uart_read
.global uart_write
.global exec
.global fork
.global exit
.global mbox_call
.global kill

getpid:
    mov x8, #0
    svc 0
    ret

uart_read:
    mov x8, #1
    svc 0
    ret

uart_write:
    mov x8, #2
    svc 0
    ret

exec:
    mov x8, #3
    svc 0

fork:
    mov x8, #4
    svc 0
    ret

exit:
    mov x8, #5
    svc 0
    ret

mbox_call:
    mov x8, #6
    svc 0
    ret

kill:
    mov x8, #7
    svc 0
    ret