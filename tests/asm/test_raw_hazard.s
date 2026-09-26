.syntax unified
.thumb
.text
.global _start
.type _start, %function

_start:
    /* Back-to-back RAW data hazard chain */
    movs r0, #0
    movs r1, #100      /* Loop 100 times */

raw_loop:
    adds r0, r0, #1    /* r0 dependency chain */
    adds r0, r0, #2
    adds r0, r0, #3
    adds r0, r0, #4
    subs r1, r1, #1
    bne raw_loop

    /* Expected r0: 100 * (1+2+3+4) = 1000 */
    movw r2, #1000
    cmp r0, r2
    bne fail

    /* Success: exit code 0 */
    movs r0, #0
    movs r7, #1
    svc #0

fail:
    movs r0, #1
    movs r7, #1
    svc #0
