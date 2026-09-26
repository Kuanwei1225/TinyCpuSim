.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Memory Stride Access Test */
    ldr r0, =stride_buf

    /* 1. Sequential Write: write 32 words (128 bytes, spanning multiple cache lines) */
    movs r1, #0        /* Index */
write_loop:
    lsls r2, r1, #2    /* Offset = index * 4 */
    adds r3, r0, r2    /* Address = base + offset */
    str r1, [r3, #0]   /* Store value = index */
    adds r1, r1, #1
    cmp r1, #32
    bne write_loop

    /* 2. Strided Read: stride = 4 words (16 bytes) */
    movs r1, #0        /* Index = 0, 4, 8, 12, ... 28 */
    movs r4, #0        /* Sum */
stride_loop:
    lsls r2, r1, #2
    adds r3, r0, r2
    ldr r5, [r3, #0]
    adds r4, r4, r5
    adds r1, r1, #4
    cmp r1, #32
    bne stride_loop

    /* Expected sum: 0 + 4 + 8 + 12 + 16 + 20 + 24 + 28 = 112 */
    cmp r4, #112
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
    .space 256
