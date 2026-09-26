.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Store-to-Load Forwarding Micro-benchmark */
    ldr r0, =fwd_buf

    movs r1, #0x42
    str r1, [r0, #0]
    ldr r2, [r0, #0]   /* Immediate load from same address (forwarding hit) */
    cmp r2, #0x42
    bne fail

    movs r1, #0x99
    str r1, [r0, #4]
    ldr r3, [r0, #4]   /* Immediate load from same address */
    cmp r3, #0x99
    bne fail

    /* Back-to-back interleaved stores and loads */
    adds r4, r2, r3    /* r4 = 0x42 + 0x99 = 0xDB (219) */
    str r4, [r0, #8]
    ldr r5, [r0, #8]
    cmp r5, #0xDB
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0

.bss
.align 4
fwd_buf:
    .space 64
