.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* 1. Basic MOV and ADD */
    movs r1, #20
    movs r2, #22
    adds r0, r1, r2    /* r0 = 42 */
    cmp r0, #42
    bne fail

    /* 2. Subtraction and comparison */
    subs r0, r0, #10   /* r0 = 32 */
    cmp r0, #32
    bne fail

    /* 3. Logic operations: AND, ORR, EOR */
    movs r1, #0xF0
    movs r2, #0x0F
    orrs r3, r1, r2    /* r3 = 0xFF */
    cmp r3, #0xFF
    bne fail

    ands r4, r1, r2    /* r4 = 0x00 */
    cmp r4, #0x00
    bne fail

    /* 4. Shift operations */
    movs r1, #1
    lsls r1, r1, #4    /* r1 = 16 */
    cmp r1, #16
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    svc #0

fail:
    movs r0, #1
    svc #0
