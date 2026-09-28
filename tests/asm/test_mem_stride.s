.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Memory Stride Access Test (>10,000 instructions) */
    ldr r0, =stride_buf
    movs r6, #20       /* Outer loop 20 rounds */
    movs r7, #0        /* Total accumulator */

outer_loop:
    /* 1. Sequential Write: write 64 words (256 bytes) */
    movs r1, #0        /* Index */
write_loop:
    lsls r2, r1, #2    /* Offset = index * 4 */
    adds r3, r0, r2    /* Address = base + offset */
    str r1, [r3, #0]   /* Store value = index */
    adds r1, r1, #1
    cmp r1, #64
    bne write_loop

    /* 2. Strided Read: stride = 4 words (16 bytes) */
    movs r1, #0        /* Index = 0, 4, 8, ... 60 */
    movs r4, #0        /* Sum */
stride_loop:
    lsls r2, r1, #2
    adds r3, r0, r2
    ldr r5, [r3, #0]
    adds r4, r4, r5
    adds r1, r1, #4
    cmp r1, #64
    bne stride_loop

    /* Expected round sum = sum(0, 4, 8, ... 60) = 4 * sum(0..15) = 4 * 120 = 480 */
    adds r7, r7, r4

    subs r6, r6, #1
    bne outer_loop

    /* Expected total: 20 * 480 = 9600 */
    movw r3, #9600
    cmp r7, r3
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
stride_buf:
    .space 512
