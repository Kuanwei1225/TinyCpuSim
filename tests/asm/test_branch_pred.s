.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Branch Prediction Pattern Test */
    /* Repeated pattern: T, NT, T, NT ... in a loop */
    movs r0, #0        /* Taken count */
    movs r1, #0        /* Not-taken count */
    movs r2, #200      /* Loop 200 iterations */

branch_loop:
    /* Check bit 0 of r2 */
    movs r3, r2
    ands r3, r3, #1
    cmp r3, #0
    beq is_even

is_odd:
    adds r0, r0, #1
    b next_iter

is_even:
    adds r1, r1, #1

next_iter:
    subs r2, r2, #1
    bne branch_loop

    /* Verify 100 odd, 100 even */
    cmp r0, #100
    bne fail
    cmp r1, #100
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0
