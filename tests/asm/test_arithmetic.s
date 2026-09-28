.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Arithmetic & Logic Comprehensive Benchmark (>2000 insts) */
    movs r6, #200       /* 200 iterations */
    movs r7, #0         /* Running checksum */

arith_loop:
    /* 1. Basic MOV and ADD */
    movs r1, #20
    adds r1, r1, r6
    movs r2, #22
    adds r0, r1, r2    /* r0 = 42 + r6 */

    /* 2. Subtraction */
    subs r0, r0, #10   /* r0 = 32 + r6 */

    /* 3. Logic operations: AND, ORR, EOR */
    movs r1, #0xF0
    movs r2, #0x0F
    orrs r3, r1, r2    /* r3 = 0xFF */
    ands r4, r1, r2    /* r4 = 0x00 */
    eors r5, r1, r2    /* r5 = 0xFF */

    /* 4. Shift & Multiply operations */
    movs r1, #1
    lsls r1, r1, #4    /* r1 = 16 */
    muls r1, r6, r1    /* r1 = 16 * r6 */

    /* 5. Accumulate into r7 */
    adds r7, r7, r0
    adds r7, r7, r3
    adds r7, r7, r5
    adds r7, r7, r1

    subs r6, r6, #1
    bne arith_loop

    cmp r7, #0
    beq fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0
